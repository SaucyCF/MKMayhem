#include <kamek.hpp>
#include <PulsarSystem.hpp>
#include <Config.hpp>
#include <Settings/SettingsParam.hpp>

namespace Pulsar {

namespace Settings {

u8 Params::radioCount[Params::pageCount] ={
    2, 5, 3, 4, 2, 5, 5, 6, 6, 6 //menu, race, host, OTT, KO, DKW1, DKW2, DKW3, DKW4, DKW5
    //Add user radio count here

};
u8 Params::scrollerCount[Params::pageCount] ={ 1, 1, 2, 0, 2, 0, 1, 2, 0, 1 }; //menu, race, host, OTT, KO, DKW1, DKW2, DKW3, DKW4, DKW5

u8 Params::buttonsPerPagePerRow[Params::pageCount][Params::maxRadioCount] = //first row is PulsarSettingsType, 2nd is rowIdx of radio
{
    { 2, 3, 0, 0, 0, 0, 0, 0 }, //Menu 
    { 2, 2, 2, 2, 3, 0, 0, 0 }, //Race
    { 2, 2, 2, 0, 0, 0, 0, 0 }, //Host
    { 3, 3, 2, 2, 0, 0, 0, 0 }, //OTT
    { 2, 2, 0, 0, 0, 0, 0, 0 }, //KO
    { 4, 4, 2, 2, 2, 0, 0, 0 }, //DKW1
    { 2, 2, 2, 3, 4, 0, 0, 0 }, //DKW2
    { 2, 2, 2, 2, 3, 4, 0, 0 }, //DKW3
    { 3, 2, 2, 2, 2, 2, 0, 0 }, //DKW4
    { 2, 2, 2, 2, 2, 2, 0, 0 }, //DKW5
};

u8 Params::optionsPerPagePerScroller[Params::pageCount][Params::maxScrollerCount] =
{
    { 5, 0, 0, 0, 0}, //Menu 
    { 4, 0, 0, 0, 0}, //Race
    { 7, 7, 0, 0, 0}, //Host
    { 0, 0, 0, 0, 0}, //OTT
    { 4, 4, 0, 0, 0}, //KO
    { 0, 0, 0, 0, 0}, //DKW1
    { 5, 0, 0, 0, 0}, //DKW2
    { 4, 4, 0, 0, 0}, //DKW3
    { 0, 0, 0, 0, 0}, //DKW4
    { 7, 0, 0, 0, 0}, //DKW5
};

}//namespace Settings
}//namespace Pulsar

//SettingParam



