#include "main.hpp"
#include "gui/gui.hpp"
#include "plant/plant.hpp"
#include "motor/motor.hpp"
#include "debug/logger.hpp"

int main(int argc, char** argv) {
    setup_log_filter();

    Logger logger;
    logger.start();

    Plant plant(logger);
    plant.start();

    Motor motor(plant, logger);
    motor.start();

    GuiApp app(motor, logger);
    int status = app.run(argc, argv);

    motor.stop();
    plant.stop();
    logger.stop();

    return status;
}