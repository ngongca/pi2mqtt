/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */
#include <err.h>
#include <string.h>
#include <signal.h>
#include <unistd.h>
#include <locale.h>
#include <confuse.h>

cfg_t *read_config(char config_filename[]) {

    cfg_t *cfg;
    static cfg_opt_t ds18b20_opts[] = {
        CFG_STR("address", "/sys/bus/w1/devices", CFGF_NONE),
        CFG_STR("mqttpubtopic", "home/device/temp", CFGF_NONE),
        CFG_INT("isfahrenheit", 1, CFGF_NONE),
        CFG_END()
    };
    static cfg_opt_t raven_opts[] = {
        CFG_STR("address", "/dev/ttyUSB0", CFGF_NONE),
        CFG_STR("mqttpubtopic", "home/device/demand", CFGF_NONE),
        CFG_END()
    };

    cfg_opt_t opts[] = {
        CFG_INT("delay", 5, CFGF_NONE),
        CFG_STR("mqttbrokeraddress", "localhost", CFGF_NONE),
        CFG_STR("mqttsubtopic", "home/+/manage", CFGF_NONE),
        CFG_STR("clientid", "id", CFGF_NONE),
        CFG_SEC("ds18b20", ds18b20_opts, CFGF_MULTI | CFGF_TITLE),
        CFG_SEC("RAVEn", raven_opts, CFGF_MULTI | CFGF_TITLE),
        CFG_END()
    };

    cfg = cfg_init(opts, 0);
    if (cfg_parse(cfg, config_filename) != CFG_SUCCESS) {
        errx(1, "Failed parsing configuration!\n");
    }
    return (cfg);
}