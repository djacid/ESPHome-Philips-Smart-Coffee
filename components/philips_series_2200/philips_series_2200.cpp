#include "esphome/core/log.h"
#include "philips_series_2200.h"

#define BUFFER_SIZE 32

namespace esphome
{
    namespace philips_series_2200
    {

        static const char *TAG = "philips_series_2200";

        void PhilipsSeries2200::setup()
        {
            power_pin_->setup();
            power_pin_->pin_mode(gpio::FLAG_OUTPUT);
            power_pin_->digital_write(true);
        }

        void PhilipsSeries2200::loop()
        {
            uint8_t buffer[BUFFER_SIZE];

            // Pipe display to mainboard
            if (display_uart_.available())
            {
                uint8_t size = std::min(display_uart_.available(), BUFFER_SIZE);
                display_uart_.read_array(buffer, size);

                mainboard_uart_.write_array(buffer, size);
                last_message_from_display_time_ = millis();
            }

            // Read until start index
            while (mainboard_uart_.available())
            {
                uint8_t buffer = mainboard_uart_.peek();
                if (buffer == 0xD5)
                    break;
                display_uart_.write(mainboard_uart_.read());
            }

            // Pipe to display
            if (mainboard_uart_.available())
            {
                uint8_t size = std::min(mainboard_uart_.available(), BUFFER_SIZE);
                mainboard_uart_.read_array(buffer, size);

                display_uart_.write_array(buffer, size);

                // Update status sensors
                for (philips_status_sensor::StatusSensor *status_sensor : status_sensors_)
                    status_sensor->update_status(buffer, size);
            }

            // Publish power state if required as long as the display is requesting messages
            if (power_switch_ != NULL)
            {
                if (millis() - last_message_from_display_time_ > POWER_STATE_TIMEOUT)
                {
                    power_switch_->publish_state(false);
                    for (philips_status_sensor::StatusSensor *status_sensor : status_sensors_)
                        status_sensor->set_state_off();
                }
                else
                {
                    power_switch_->publish_state(true);
                }
            }

            display_uart_.flush();
            mainboard_uart_.flush();
        }

        void PhilipsSeries2200::dump_config()
        {
            ESP_LOGCONFIG(TAG, "Philips Series 2200");
            display_uart_.check_uart_settings(115200, 1, uart::UART_CONFIG_PARITY_NONE, 8);
            mainboard_uart_.check_uart_settings(115200, 1, uart::UART_CONFIG_PARITY_NONE, 8);
        }

    } // namespace philips_series_2200
} // namespace esphome