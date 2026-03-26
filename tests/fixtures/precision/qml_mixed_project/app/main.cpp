#include "../backend/facade/TrafficBackend.h"

int main() {
    App::Backend::TrafficBackend backend;
    return backend.refresh();
}
