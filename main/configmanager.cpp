#include "includes.h"

static const char * tag = "CFG";

ConfigManager ConfigManager::s_instance;

ConfigManager::ConfigManager(){

}

ConfigManager::~ConfigManager(){

}

ConfigManager *ConfigManager::getInstance(){
    return &s_instance;
}