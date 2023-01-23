#include "common.h"
#include "interceptutils.h"

int main(int argc, char* argv[]){
    auto parsedMap = utils::parseArgs(argc, argv);
    DEBUG_PARSED_ARGS(parsedMap);

    auto modeData = utils::getProcessModeData(parsedMap);
    DEBUG_MODE_DATA(modeData);

    // Map is no longer needed, free memory
    parsedMap->clear();
    delete parsedMap;
    utils::execAsPerMode(modeData);
    return 0;
}