#include <string>

namespace Atlas {
    struct Err {
        bool success;
        std::string message;
        
        explicit operator bool() const {
            return success;
        }
    };
    Err Success() {
        return {true, ""};
    }
    Err Failure(const std::string& msg) {
        return {false, msg};
    };
}

