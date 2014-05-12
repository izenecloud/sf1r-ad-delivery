#ifndef ORI_DOCUMENT_
#define ORI_DOCUMENT_
#include<string>
namespace sf1r
{
namespace laser
{
namespace clustering{
    struct OriDocument
    {
        std::string title;
        std::string category;
        std::string docid;
        OriDocument(std::string t, std::string c, std::string d)
          :title(t),category(c),docid(d)
        {
        }

    };
}
}
}
#endif
