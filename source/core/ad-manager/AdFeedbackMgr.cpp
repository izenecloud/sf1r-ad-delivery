#include "AdFeedbackMgr.h"
#include <glog/logging.h>
#include <3rdparty/rapidjson/reader.h>
#include <3rdparty/rapidjson/document.h>
#include <avro/Decoder.hh>
#include <avro/Compiler.hh>
#include <avro/Generic.hh>
#include <avro/Specific.hh>
#include <avro/Encoder.hh>
#include <avro/Stream.hh>
#include "B5MEvent.hh"

#define USER_PROFILE_CACHE_SECS 60*10

static avro::ValidSchema log_schema;

struct AvroLog
{
    int64_t timestamp;
    std::string client_ip;
    std::string forward_address;
    std::string user_agent;
    std::map<std::string, std::string> args;
    std::map<std::string, std::string> internal_args;
    std::string body;
};

namespace avro{
    template <>
        struct codec_traits<AvroLog> {
            static void encode(Encoder& e, const AvroLog& c)
            {
                avro::encode(e, c.timestamp);
                avro::encode(e, c.client_ip);
                avro::encode(e, c.forward_address);
                avro::encode(e, c.user_agent);
                avro::encode(e, c.args);
                avro::encode(e, c.internal_args);
                avro::encode(e, c.body);
            }
            static void decode(Decoder& d, AvroLog& c)
            {
                avro::decode(d, c.timestamp);
                avro::decode(d, c.client_ip);
                avro::decode(d, c.forward_address);
                avro::decode(d, c.user_agent);
                avro::decode(d, c.args);
                avro::decode(d, c.internal_args);
                avro::decode(d, c.body);
            }
        };
}


namespace sf1r
{

static bool convertToUserProfile(const std::string& data, AdFeedbackMgr::UserProfile& user_profile)
{
    // TODO:convert the json string data to user_profile and cache it.
    return true;
}

static void get_callback(lcb_t instance, const void* cookie, lcb_error_t error, const lcb_get_resp_t *resp)
{
    if (error == LCB_SUCCESS)
    {
        std::string body((const char*)resp->v.v0.bytes, resp->v.v0.nbytes);
        AdFeedbackMgr::get()->setDMPRsp(std::string((const char*)cookie), body);
    }
    else
    {
        LOG(WARNING) << "couchbase get error." << lcb_strerror(instance, error);
    }
}

static void err_callback(lcb_t instance, lcb_error_t error, const char* info)
{
    LOG(WARNING) << "error in couchbase callback: " << lcb_strerror(instance, error);
}

lcb_t AdFeedbackMgr::get_conn_from_pool()
{
    lcb_t ret;
    {
        boost::unique_lock<boost::mutex> dmp_pool_lock_;
        if (!dmp_conn_pool_.empty())
        {
            ret = dmp_conn_pool_.front();
            dmp_conn_pool_.pop_front();
            return ret;
        }
    }

    lcb_error_t err;
    struct lcb_create_st options;
    memset(&options, 0, sizeof(options));
    options.v.v0.host = dmp_server_ip_.c_str();
    options.v.v0.user = "user_profile";
    options.v.v0.passwd = "";
    options.v.v0.bucket = "user_profile";
    err = lcb_create(&ret, &options);
    if (err != LCB_SUCCESS)
    {
        LOG(ERROR) << "failed to create couchbase connection." << lcb_strerror(NULL, err);
        return NULL;
    }
    lcb_set_error_callback(ret, err_callback);
    err = lcb_connect(ret);
    if (err != LCB_SUCCESS)
    {
        LOG(ERROR) << "failed to connect to couchbase." << lcb_strerror(NULL, err);
        lcb_destroy(ret);
        ret = NULL;
        return ret;
    }
    lcb_wait(ret);
    // usec for timeout.
    lcb_set_timeout(ret, 1000*100);
    lcb_set_get_callback(ret, get_callback);
    LOG(INFO) << "a new connection to DMP established";
    return ret;
}

void AdFeedbackMgr::free_conn_to_pool(lcb_t conn)
{
    boost::unique_lock<boost::mutex> dmp_pool_lock_;
    if (conn)
    {
        dmp_conn_pool_.push_back(conn);
    }
}

AdFeedbackMgr::AdFeedbackMgr()
    :cached_user_profiles_(100000, izenelib::cache::LRLFU)
{
}

AdFeedbackMgr::~AdFeedbackMgr()
{
    for(std::list<lcb_t>::const_iterator it = dmp_conn_pool_.begin();
        it != dmp_conn_pool_.end(); ++it)
    {
        if (*it != NULL)
        {
            lcb_destroy(*it);
        }
    }
    dmp_conn_pool_.clear();
}

void AdFeedbackMgr::init(const std::string& dmp_server_ip, uint16_t port, const std::string& schema_path)
{
    dmp_server_ip_ = dmp_server_ip;
    dmp_server_port_ = port;

    std::ifstream ifs(schema_path.c_str());
    std::string errinfo;
    if (!avro::compileJsonSchema(ifs, log_schema, errinfo))
    {
        LOG(WARNING) << "schema error." << errinfo << ", file :" << schema_path;
    }
}

void AdFeedbackMgr::setDMPRsp(const std::string& cookie, const std::string& data)
{
    boost::unique_lock<boost::mutex> dmp_rsp_lock_;
    if (data.empty())
        return;
    tmp_rsp_list_[cookie] = data;
}

bool AdFeedbackMgr::getUserProfileFromDMP(const std::string& user_id, UserProfile& user_profile)
{
    if (cached_user_profiles_.get(user_id, user_profile))
    {
        if (std::time(NULL) < (user_profile.timestamp + USER_PROFILE_CACHE_SECS))
        {
            return true;
        }
    }
    user_profile.profile_data.clear();
    lcb_t conn = get_conn_from_pool();
    if (conn == NULL)
        return false;
    lcb_error_t err;
    lcb_get_cmd_t cmd;
    const lcb_get_cmd_t *commands[1];
    commands[0] = &cmd;
    memset(&cmd, 0, sizeof(cmd));
    cmd.v.v0.key = user_id.c_str();
    cmd.v.v0.nkey = strlen((const char*)cmd.v.v0.key);
    err = lcb_get(conn, user_id.c_str(), 1, commands);
    bool result = true;
    if (err == LCB_SUCCESS)
    {
        lcb_wait(conn);
        boost::unique_lock<boost::mutex> dmp_rsp_lock_;
        std::map<std::string, std::string>::const_iterator rsp_it = tmp_rsp_list_.find(user_id);
        if (rsp_it == tmp_rsp_list_.end())
        {
            LOG(WARNING) << "DMP no response for user : " << user_id;
            result = false;
        }
        else
        {
            if(!convertToUserProfile(rsp_it->second, user_profile))
            {
                LOG(WARNING) << "DMP response data convert to user profile failed.";
                result = false;
            }
            else
            {
                user_profile.timestamp = time(NULL);
                cached_user_profiles_.insert(user_id, user_profile);
            }
        }
    }
    else
    {
        LOG(WARNING) << "failed to get user profile : " << user_id << ", " << lcb_strerror(NULL, err);
        result = false;
    }

    if (err == LCB_NETWORK_ERROR ||
        err == LCB_CONNECT_ERROR)
    {
        LOG(WARNING) << "a DMP connection destroyed for connection lost.";
        lcb_destroy(conn);
        return result;
    }
    free_conn_to_pool(conn);
    return result;
}

bool AdFeedbackMgr::parserFeedbackLog(const std::string& log_data, FeedbackInfo& feedback_info)
{
    namespace rj = rapidjson;
    // extract the user id, ad id and action from log
    rj::Document log_doc;
    bool err = log_doc.Parse<0>(log_data.c_str()).HasParseError();
    if (err)
    {
        LOG(ERROR) << "parsing json log data error.";
        return false;
    }
    if (!log_doc.HasMember("args"))
    {
        LOG(ERROR) << "log data has no args.";
        return false;
    }
    const rj::Value& args = log_doc["args"];
    if (!args.IsObject())
    {
        LOG(ERROR) << "log args is not object.";
        return false;
    }
    if (!args.HasMember("ad") ||
        !args.HasMember("uid") ||
        !args.HasMember("dstl") ||
        !args.HasMember("aid"))
    {
        // some of necessary value missing, just ignore.
        return false;
    }
    feedback_info.user_id = args["uid"].GetString();
    feedback_info.ad_id = args["aid"].GetString();
    std::string action_str = args["ad"].GetString();
    if (action_str == "103")
        feedback_info.action = Click;
    else if (action_str == "108")
        feedback_info.action = View;
    else
    {
        LOG(INFO) << "unknow action string: " << action_str;
        feedback_info.action = View;
    }

    if (feedback_info.action == Click)
    {
        if (args.HasMember("click_cost"))
        {
            feedback_info.click_cost = args["click_cost"].GetDouble();
        }
        if (args.HasMember("click_slot"))
        {
            feedback_info.click_slot = args["click_slot"].GetInt();
        }
    }
    if (feedback_info.user_id.empty() &&
        feedback_info.ad_id.empty())
    {
        LOG(INFO) << "empty user id and ad id." << feedback_info.user_id << ":" << feedback_info.ad_id;
        return false;
    }

    if (feedback_info.user_id.empty())
    {
        // no user data for this log.
        return true;
    }
    return getUserProfileFromDMP(feedback_info.user_id, feedback_info.user_profiles);
}
bool AdFeedbackMgr::parserFeedbackLogForAVRO(const std::string& log_data, FeedbackInfo& feedback_info)
{
    //AvroLog test_data;
    //test_data.timestamp = 1393992000;
    //test_data.client_ip = "14.135.156.84";
    //test_data.forward_address = "-";
    //test_data.user_agent = "Mozilla/5.0";
    //std::map<std::string, std::string> args;
    //args["uid"] = "49019dc4d931a5de98a6f792608f6b74";
    //args["ad"] = "108";
    //args["dstl"] = "http:://s.b5m.com/cpc/item/123.html";
    //
    //test_data.args = args;
    //test_data.internal_args["region"] = "21";
    //test_data.internal_args["city"] = "Yinchuan";
    //test_data.body = "";

    //std::auto_ptr<avro::OutputStream> out = avro::memoryOutputStream();
    //avro::EncoderPtr en = avro::jsonEncoder(log_schema);
    //en->init(*out);

    //avro::encode(*en, test_data);
    //uint8_t* tmp;
    //std::size_t len;
    //out->next(&tmp, &len);
    //AvroLog test_result;
    //std::auto_ptr<avro::InputStream> test_in = avro::memoryInputStream(*out);
    //avro::DecoderPtr test_d = avro::jsonDecoder(log_schema);
    //test_d->init(*test_in);
    //avro::decode(*test_d, test_result);

    //AvroLog raw_data;
    //std::auto_ptr<avro::InputStream> in = avro::memoryInputStream((const uint8_t*)log_data.data(), log_data.size());
    //avro::DecoderPtr d = avro::jsonDecoder(log_schema);
    //avro::DecoderPtr d = avro::binaryDecoder();
    //d->init(*in);

    //try
    //{
    //    avro::decode(*d, raw_data);
    //}
    //catch(const std::exception& e)
    //{
    //    LOG(WARNING) << "decoding avro log data failed." << e.what();
    //    return false;
    //}
    B5MEvent raw_data;
    avro::OutputBuffer out;
    out.writeTo(log_data.data(), log_data.size());
    avro::InputBuffer buf(out);
    avro::Reader reader(buf);
    avro::parse(reader, raw_data);

    Map_of_string::MapType& args_v = raw_data.args.value;
    feedback_info.user_id = args_v["uid"];
    feedback_info.ad_id = args_v["aid"];
    if ("103" == args_v["ad"])
    {
        feedback_info.action = Click;
    }
    else
    {
        feedback_info.action = View;
    }

    if (feedback_info.action == Click)
    {
        try
        {
            feedback_info.click_cost = boost::lexical_cast<double>(args_v["click_cost"]);
            feedback_info.click_slot = boost::lexical_cast<uint32_t>(args_v["click_slot"]);
        }
        catch(const std::exception& e)
        {
        }
    }

    if (!feedback_info.user_id.empty() && !feedback_info.ad_id.empty())
        return getUserProfileFromDMP(feedback_info.user_id, feedback_info.user_profiles);

    //avro::GenericDatum datum(log_schema);
    //avro::decode(*d, datum);
    //LOG(INFO) << "Type: " << datum.type();

    //if (datum.type() == avro::AVRO_RECORD)
    //{
    //    const avro::GenericRecord& r = datum.value<avro::GenericRecord>();
    //    LOG(INFO) << "Field-count:" << r.fieldCount();
    //    const avro::GenericDatum& args = r.field("args");
    //    if (args.type() == avro::AVRO_MAP)
    //    {
    //        typedef std::map<std::string, std::string> ArgsMapT;
    //        const ArgsMapT& kv = args.value<ArgsMapT>();
    //        for(ArgsMapT::const_iterator it = kv.begin(); it != kv.end(); ++it)
    //        {
    //            LOG(INFO) << it->first << ":" << it->second;
    //        }
    //        ArgsMapT::const_iterator it = kv.find("uid");
    //        if (it != kv.end())
    //        {
    //            feedback_info.user_id = it->second;
    //        }
    //        it = kv.find("aid");
    //        if (it != kv.end())
    //        {
    //            feedback_info.ad_id = it->second;
    //        }
    //        it = kv.find("ad");
    //        if (it != kv.end())
    //        {
    //            if (it->second == "103")
    //            {
    //                feedback_info.action = Click;
    //            }
    //            else
    //            {
    //                feedback_info.action = View;
    //            }
    //        }
    //        if (!feedback_info.user_id.empty() && !feedback_info.ad_id.empty())
    //        {
    //            return getUserProfileFromDMP(feedback_info.user_id, feedback_info.user_profiles);
    //        }
    //    }
    //    else
    //    {
    //        LOG(WARNING) << "args type error." << args.type();
    //    }
    //}
    return false;
}

}

