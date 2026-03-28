#include "host/PluginHost.h"

#include "plugins/metrics/MetricsPlugin.h"

int PluginHost::start() const {
    MetricsPlugin plugin;
    return plugin.activate();
}
