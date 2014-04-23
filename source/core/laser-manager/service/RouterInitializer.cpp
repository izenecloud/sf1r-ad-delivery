#include "RouterInitializer.h"
#include "predict/TopNController.h"

void initializeRouter(izenelib::driver::Router& router)
{
//TODO
    {
        predict::TopNController topn;
        const std::string controllerName("topn");
        typedef ::izenelib::driver::ActionHandler<predict::TopNController> handler_type;
        typedef std::auto_ptr<handler_type> handler_ptr;

        handler_ptr indexHandler(
            new handler_type(
                topn,
                &predict::TopNController::top
            )
        );

        router.map(
            controllerName,
            "topn",
            indexHandler.get()
        );
        indexHandler.release();
    }
}

