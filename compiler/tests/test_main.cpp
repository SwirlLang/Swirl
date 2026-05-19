#include <catch2/catch_session.hpp>

bool SW_IS_DEBUG = false;

int main(int argc, char* argv[]) {
    Catch::Session session;
    return session.run(argc, argv);
}
