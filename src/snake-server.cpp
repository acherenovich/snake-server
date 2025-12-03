#include <logging.hpp>

int main() {
    auto log = Utils::Logging::Logger::Create("CORE");

    log->Error("Hello error!");
    log->Warning("Hello warning!");
    log->Debug("Hello debug!");
    log->Msg("Hello msg!");

    log->CreateChild("FORMAT")->Msg("Hello format({} -> {})!", 1, "aaa");
    log->Fatal("Hello fatal!");
}