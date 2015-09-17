#ifndef WINDOW_HPP
#define WINDOW_HPP

#include <functional>
#include <future>
#include <map>
#include <mutex>
#include <stdexcept>
#include <thread>
#include <vector>

#include <experimental/optional>

#include <boost/iterator/transform_iterator.hpp>
#include <boost/variant.hpp>

#include "ppapi/cpp/instance.h"
#include "ppapi/cpp/message_loop.h"
#include "ppapi/cpp/message_handler.h"

#include "jsvar.hpp"

class DomPepper;

class Window : private pp::MessageHandler {
    public:
        using callback_type = std::function<void (const pp::VarArray&)>;

        explicit Window(pp::Instance& instance_)
            : instance{instance_},
              handler_thread{std::bind(&Window::thread_fn, this)},
              embed{std::make_shared<jsref::id_type>(0)} {}

        auto jsffi(const jsvar& obj, const std::string& method, const std::experimental::optional<std::vector<jsvar>>& args = {}, const bool ret = true) -> jsvar {
            auto lock = ret ? std::unique_lock<std::mutex>{mutex} : std::unique_lock<std::mutex>{};

            if (ret)
                retval = decltype(retval){};

            pp::VarDictionary dict;
            dict.Set("obj", marshall(obj));
            dict.Set("method", method);
            if (args)
                dict.Set("args", make_vararray(boost::make_transform_iterator(std::begin(*args), Window::marshall),
                                               boost::make_transform_iterator(std::end  (*args), Window::marshall)));
            dict.Set("ret", ret);
            instance.PostMessage(dict);
            
            if (ret)
                return retval.get_future().get();
            else
                return {};
        }

        auto register_callback(callback_type fn) -> jsref {
            const auto id = boost::get<pp::Var>(jsffi(embed, "registerCallback", {{}}).var).AsInt();
            callbacks.emplace(std::make_pair(id, fn));
            return {std::make_shared<jsref::id_type>(id)};
        }

        void alert(const std::string& s) {
            jsffi({}, "alert", {{s}}, false);
        }
        
        auto console() -> jsref {
            return boost::get<jsref>(jsffi({}, "console").var);
        }
        
        auto document() -> jsref {
            return boost::get<jsref>(jsffi({}, "document").var);
        }

        auto documentGetElementById(const jsref& document, const std::string& s) -> jsref {
            return boost::get<jsref>(jsffi(document, "getElementById", {{s}}).var); 
        }

        void eventTargetAddEventListener(const jsref& target, const std::string& type, const jsref& listener) {
            jsffi(target, "addEventListener", {{type, listener}}, false);
        }

        void consoleLog(const jsref& console, const std::string& s) {
            jsffi(console, "log", {{s}}, false);
        }

    private:
        virtual auto HandleBlockingMessage(pp::InstanceHandle, const pp::Var&) -> pp::Var override { return {}; } 
        virtual void HandleMessage(pp::InstanceHandle, const pp::Var& var) override {
            const auto& dict = pp::VarDictionary{var};
            if (dict.Get("type").AsString() == "call") {
                std::thread t([=] {
                    callbacks[dict.Get("id").AsInt()](pp::VarArray{dict.Get("data")});
                });
                t.detach();
            } else {
                retval.set_value(demarshall(var));
            }
        } 

        virtual void WasUnregistered(pp::InstanceHandle) override {
            mutex.lock();
        }

        void destroy_jsref(const jsref::id_type id) {
            jsffi(embed, "destroyJsref", {{pp::Var(id)}}, false);
        }

        void thread_fn() {
            pp::MessageLoop msgloop(&instance);
            msgloop.AttachToCurrentThread();
            instance.RegisterMessageHandler(this, msgloop);
            msgloop.Run();
        }

        struct marshall_visitor : boost::static_visitor<pp::Var> {
            auto operator()(const pp::Var& v) const {
                return make_vararray({v});
            }

            auto operator()(const pp::VarArray& v) const {
                return make_vararray({v});
            }

            auto operator()(const pp::VarDictionary& v) const {
                return make_vararray({v});
            }

            auto operator()(const jsref& v) const {
                return v.id();
            }
        };

        static pp::Var marshall(const jsvar& v) {
            return boost::apply_visitor(marshall_visitor{}, v.var);
        }

        jsvar demarshall(const pp::Var& v) {
            const auto& dict = pp::VarDictionary{v};
            const auto& type = dict.Get("type").AsString();
            const auto& data = dict.Get("data");
            if (type == "raw") {
                if (data.is_array()) {
                    return pp::VarArray{data};
                } else if (data.is_dictionary()) {
                    return pp::VarDictionary{data};
                } else {
                    return data;
                }
            } else if (type == "jsref") {
                return jsref{{new jsref::id_type(data.AsInt()), [&](const auto p) { this->destroy_jsref(*p); delete p; }}};
            } else {
                throw std::runtime_error("Demarhsall error");
            }
        }

        pp::Instance& instance;
        std::thread handler_thread;
        std::mutex mutex;
        std::promise<const jsvar> retval;
        jsref embed;
        std::map<std::int32_t, callback_type> callbacks;
};

#endif
