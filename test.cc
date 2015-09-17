#include <chrono>
#include <condition_variable>
#include <iostream>
#include <mutex>
#include <string>
#include <thread>

#include "ppapi/cpp/instance.h"
#include "ppapi/cpp/module.h"
#include "ppapi/cpp/var.h"
#include "ppapi/cpp/var_array.h"
#include "ppapi/cpp/var_dictionary.h"

#include "window.hpp"

using namespace std::literals;

class TestInstance : public pp::Instance {
    public:
        explicit TestInstance(const PP_Instance instance)
            : pp::Instance{instance}, window{*this}
        {
            auto mutex = std::make_shared<std::mutex>();
            auto cv = std::make_shared<std::condition_variable>();
            auto cond = std::make_shared<bool>(false);

            std::thread t1([&, mutex, cv, cond] {
                window.alert("hello from thread1");
                
                {
                    std::unique_lock<std::mutex> lock(*mutex);
                    cv->wait(lock, [cond] { return *cond; });
                }

                const auto console = window.console();
                for (int i = 0; ; ++i) {
                    window.consoleLog(console, "umbilicus" + std::to_string(i));
                    std::this_thread::sleep_for(100ms);
                }
            });

            t1.detach();
            
            std::thread t2([&, mutex, cv, cond] {
                window.alert("hello from thread2");

                {
                    std::unique_lock<std::mutex> lock(*mutex);
                    cv->wait(lock, [cond] { return *cond; });
                }

                const auto console = window.console();
                for (int i = 0; ; ++i) {
                    window.consoleLog(console, "credict" + std::to_string(i));
                    std::this_thread::sleep_for(300ms);
                }
            });

            t2.detach();

            std::thread t3([&, mutex, cv, cond] {
                window.alert("hello from thread3");
                const auto listener = window.register_callback([&, mutex, cv, cond](const pp::VarArray&) {
                    window.alert("alert() from nacl listener");
                    std::unique_lock<std::mutex> lock(*mutex);
                    *cond = true;
                    cv->notify_one();
                });

                const auto document = window.document();
                const auto button = window.documentGetElementById(document, "title");
                window.eventTargetAddEventListener(button, "click", listener);
            });
            
            t3.detach();
        }

        virtual ~TestInstance() = default;
        
    private:
        Window window;
};

class TestModule : public pp::Module {
    public:
        virtual ~TestModule() = default;

        virtual auto CreateInstance(PP_Instance instance) -> pp::Instance* {
            return new TestInstance(instance);
        }
};

namespace pp {
    auto CreateModule() {
        return new TestModule();
    }
}
