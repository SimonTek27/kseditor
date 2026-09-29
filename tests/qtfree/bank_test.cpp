#include "KsTest.h"
#include "Audio/BankParserBridge.h"

#include <filesystem>
#include <fstream>
#include <string>

int main() {
    const ks::audio::ParsedBank missing =
        ks::audio::parseBankFile("definitely_missing_ks_qtfree_test.bank");
    KS_CHECK(!missing.valid);
    KS_CHECK(missing.events.empty());
    KS_CHECK(missing.sounds.empty());

    const ks::audio::ParsedBank empty = ks::audio::parseBankFile("");
    KS_CHECK(!empty.valid);
    KS_CHECK(empty.events.empty());

    const std::filesystem::path p =
        std::filesystem::temp_directory_path() / "ks_qtfree_garbage.bank";
    {
        std::ofstream out(p, std::ios::binary);
        const char junk[] = "NOTAFMODBANK0123456789";
        out.write(junk, sizeof(junk));
    }
    const ks::audio::ParsedBank garbage = ks::audio::parseBankFile(p.string());
    KS_CHECK(!garbage.valid);
    KS_CHECK(garbage.events.empty());
    KS_CHECK(garbage.sounds.empty());
    std::filesystem::remove(p);

    const ks::audio::ParsedBank directory =
        ks::audio::parseBankFile(std::filesystem::temp_directory_path().string());
    KS_CHECK(!directory.valid);

    return KS_TEST_RESULT("bank_test");
}
