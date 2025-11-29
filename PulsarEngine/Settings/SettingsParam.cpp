#include <kamek.hpp>
#include <PulsarSystem.hpp>
#include <Config.hpp>
#include <Settings/SettingsParam.hpp>

namespace Pulsar {

namespace Settings {

u8 Params::radioCount[Params::pageCount] ={
    6, 6, 6, 6, 4, 5, 2, 1 //Misc, Misc2, Misc3, Rules, Rules2, Item, KO, WW
    //Add user radio count here

};
u8 Params::scrollerCount[Params::pageCount] ={ 1, 1, 1, 2, 2, 0, 2, 1 }; //Misc, Misc2, Misc3, Rules, Rules2, Item, KO, WW

u8 Params::buttonsPerPagePerRow[Params::pageCount][Params::maxRadioCount] = //first row is PulsarSettingsType, 2nd is rowIdx of radio
{
    { 2, 2, 2, 2, 2, 3, 0, 0 }, //Misc
    { 4, 3, 2, 2, 2, 2, 0, 0 }, //Misc2
    { 2, 2, 2, 2, 2, 2, 0, 0 }, //Misc3
    { 2, 2, 2, 2, 2, 2, 0, 0 }, //Rules
    { 2, 2, 3, 4, 0, 0, 0, 0 }, //Rules2
    { 4, 2, 2, 2, 2, 0, 0, 0 }, //Item
    { 3, 2, 0, 0, 0, 0, 0, 0 }, //KO
    { 3, 0, 0, 0, 0, 0, 0, 0 }, //WW
};

u8 Params::optionsPerPagePerScroller[Params::pageCount][Params::maxScrollerCount] =
{
    { 5, 0, 0, 0, 0}, //Misc
    { 4, 0, 0, 0, 0}, //Misc2
    { 3, 0, 0, 0, 0}, //Misc3
    { 7, 7, 0, 0, 0}, //Rules
    { 4, 6, 0, 0, 0}, //Rules2
    { 0, 0, 0, 0, 0}, //Item
    { 4, 4, 0, 0, 0}, //KO
    { 4, 0, 0, 0, 0}, //WW
};

}//namespace Settings
}//namespace Pulsar

//SettingParam .



