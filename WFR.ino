/*
 * @author: Polyakov Konstantin
 * 
 * NTP-серверы:
 *   time.nist.gov
 *   pool.ntp.org
 * https://github.com/gmag11/NtpClient/blob/master/examples/NTPClientESP8266/NTPClientESP8266.ino
 * https://ru.wikipedia.org/wiki/NTP
 * https://esp8266.ru/forum/threads/sinxronizacija-chasov.1951/page-2
 * 
 * 74HC595 - http://arduino.ru/Tutorial/registr_74HC595
 */



#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <EEPROM.h>
#include <Ticker.h>
//#include "FS.h"
#include <DNSServer.h>
#include <WFRChannels.h>
#include <ArduinoJson.h>

#include <DS1307RTC.h>
#include <Time.h>

// 0 - simple, 1 - advanced
#define INTERFACE_TYPE main
#define COUNT_OUTLETS 3

ADC_MODE(ADC_VCC);

WFRChannels wfr_channels;
ESP8266WebServer webServer(80);
DNSServer dnsServer;

Ticker schedule;

byte pin_btn_reset = 14;

const byte pin_i2c_sda = 4;
const byte pin_i2c_scl = 5;

// shift registr (example - 74hc595)
const byte pin_shiftr_clock = 13;
const byte pin_shiftr_latch = 12;
const byte pin_shiftr_data = 16;

Ticker t_gpio0;
Ticker t_gpio1;
Ticker t_gpio2;
Ticker t_gpio3;
Ticker t_gpio4;
Ticker t_gpio5;
Ticker t_gpio6;
Ticker t_gpio7;
Ticker t_gpio8;
Ticker t_gpio9;
Ticker t_gpio10;
Ticker t_gpio11;
Ticker t_gpio12;
Ticker t_gpio13;
Ticker t_gpio14;
Ticker t_gpio15;
Ticker timers[16] = {t_gpio0, t_gpio1, t_gpio2, t_gpio3, t_gpio4, t_gpio5, t_gpio6, t_gpio7, t_gpio8, t_gpio9, t_gpio10, t_gpio11, t_gpio12, t_gpio13, t_gpio14, t_gpio15};
Ticker d_gpio0;
Ticker d_gpio1;
Ticker d_gpio2;
Ticker d_gpio3;
Ticker d_gpio4;
Ticker d_gpio5;
Ticker d_gpio6;
Ticker d_gpio7;
Ticker d_gpio8;
Ticker d_gpio9;
Ticker d_gpio10;
Ticker d_gpio11;
Ticker d_gpio12;
Ticker d_gpio13;
Ticker d_gpio14;
Ticker d_gpio15;
Ticker delay_press[16] = {d_gpio0, d_gpio1, d_gpio2, d_gpio3, d_gpio4, d_gpio5, d_gpio6, d_gpio7, d_gpio8, d_gpio9, d_gpio10, d_gpio11, d_gpio12, d_gpio13, d_gpio14, d_gpio15};


// состояния каналов реле мы не храним в структуре, а отдельно,
// чтобы при каждом изменении сотсояния не трогать настройки.
struct DefaultSettings {
    byte wifi_mode = 0; // 0 - точка доступа, 1 - клиент, 2 - оба варианта одновременно

    char ssidAP[32] = "WiFi_Relay";
    char passwordAP[32] = "";
    //byte use_passwordAP = 0;

    char ssid[32] = "Your name";
    char password[32] = "Your password";

    char device_name[64] = "Device name";

    unsigned int update_time = 2000;

    // 4096 - 200
    char bt_panel[3384] = "[{\"name\":\"Кабинет\",\"children\":[{\"type\":\"button\",\"name\":\"Главный свет\",\"ch_name\":\"6\"},{\"type\":\"group\",\"name\":\"Настольный свет\",\"children\":[{\"type\":\"button\",\"name\":\"1\",\"ch_name\":\"0\"},{\"type\":\"button\",\"name\":\"2\",\"ch_name\":\"1\"},{\"type\":\"button\",\"name\":\"3\",\"ch_name\":\"2\"},{\"type\":\"button\",\"name\":\"4\",\"ch_name\":\"3\"},{\"type\":\"button\",\"name\":\"нижний\",\"ch_name\":\"4\"}]}]},{\"name\":\"Кухня\",\"children\":[{\"type\":\"group\",\"name\":\"Газовая плита\",\"children\":[{\"type\":\"button\",\"name\":\"1\",\"ch_name\":\"7\"},{\"type\":\"button\",\"name\":\"2\",\"ch_name\":\"8\"},{\"type\":\"button\",\"name\":\"3\",\"ch_name\":\"9\"},{\"type\":\"button\",\"name\":\"4\",\"ch_name\":\"10\"},{\"type\":\"button\",\"name\":\"Духовка\",\"ch_name\":\"11\"}]},{\"type\":\"button\",\"name\":\"Главный свет\",\"ch_name\":\"12\"},{\"type\":\"button\",\"name\":\"Вытяжка\",\"ch_name\":\"13\"}]}]";
    // [{"name":"Кабинет","children":[{"type":"button","name":"Главный свет","ch_name":"6"},{"type":"group","name":"Настольный свет","children":[{"type":"button","name":"1","ch_name":"0"},{"type":"button","name":"2","ch_name":"1"},{"type":"button","name":"3","ch_name":"2"},{"type":"button","name":"4","ch_name":"3"},{"type":"button","name":"нижний","ch_name":"4"}]}]},{"name":"Кухня","children":[{"type":"group","name":"Газовая плита","children":[{"type":"button","name":"1","ch_name":"7"},{"type":"button","name":"2","ch_name":"8"},{"type":"button","name":"3","ch_name":"9"},{"type":"button","name":"4","ch_name":"10"},{"type":"button","name":"Духовка","ch_name":"11"}]},{"type":"button","name":"Главный свет","ch_name":"12"},{"type":"button","name":"Вытяжка","ch_name":"13"}]}]
};

// eeprom addresses
const unsigned int ee_addr_start_firstrun = 0;
const unsigned int ee_addr_start_channels = 1;
const unsigned int ee_addr_start_settings = 5;
//const unsigned int ee_addr_start_bt_panel = ee_addr_start_settings + sizeof(DefaultSettings);

const byte code_firstrun = 2;

struct WFRStatistic {
    float vcc = 0;
    String time_s = "00";
    String time_m = "00";
    String time_h = "00";
    String rtc_s = "00";
    String rtc_m = "00";
    String rtc_h = "00";
    String rtc_day = "00";
    String rtc_month = "00";
    String rtc_year = "0000";
    String rtc_is = "false";
};

DefaultSettings ee_data;
WFRStatistic stat;

byte is_wifi_client_connected = 0;

void statistic_update(void) {

    // время работы
  
    unsigned long t = millis()/1000;
    byte h = t/3600;
    byte m = (t-h*3600)/60;
    unsigned int s = (t-h*3600-m*60);

    stat.time_h = (h>9?"":"0") + String(h, DEC);
    stat.time_m = (m>9?"":"0") + String(m, DEC);
    stat.time_s = (s>9?"":"0") + String(s, DEC);

    // напряжение

    stat.vcc = ((float)(ESP.getVcc()))/1000 - 0.19;

    // текущее время

    TimeElements te;
    time_t timeVal = makeTime(te);
    //RTC.set(timeVal);
    setTime(timeVal);

    setSyncProvider(RTC.get);   // получаем время с RTC
    if (timeStatus() != timeSet) setSyncProvider(RTC.get);
    if (timeStatus() == timeSet) stat.rtc_is = "true";
    else stat.rtc_is = "false";

    byte _h = hour();
    byte _m = minute();
    byte _s = second();
    byte _day = day();
    byte _month = month();
    int _year = year();
    stat.rtc_h = (_h>9?"":"0") + String(_h, DEC);
    stat.rtc_m = (_m>9?"":"0") + String(_m, DEC);
    stat.rtc_s = (_s>9?"":"0") +  String(_s, DEC);
    stat.rtc_day = (_day>9?"":"0") + String(_day, DEC);
    stat.rtc_month = (_month>9?"":"0") + String(_month, DEC);
    stat.rtc_year = String((_year>999?"":"0")) + String((_year>99?"":"0")) + String((_year>9?"":"0")) + String(_year, DEC);
}

void notFoundHandler() {
    webServer.send(404, "text/html", "<h1>Not found :-(</h1>");
}

void Timer_channel_write(int data) {
    wfr_channels.write(data>>8, data & 255);
}

//Ticker delay_press;
//Ticker timer;
void apiHandler() {

    String action = webServer.arg("action");

    DynamicJsonBuffer jsonBuffer;
    JsonObject& answer = jsonBuffer.createObject();
    answer["success"] = 1;
    answer["message"] = "Успешно!";
    JsonObject& data = answer.createNestedObject("data");

    if (action == "gpio") {

        String msg_d, msg_t;

        int channel = webServer.arg("channel").toInt();
        int value = webServer.arg("value").toInt();
        int sec_timer = webServer.arg("timer").toInt();
        int sec_delay_press = webServer.arg("delay_press").toInt();

        if (sec_timer != 0) {
            int cur_value = wfr_channels.read(channel);
            timers[channel].once(sec_timer + sec_delay_press, Timer_channel_write, (channel<<8) + cur_value);
            msg_t = "через "+ String(sec_timer, DEC) +" сек.";
        }
        if (sec_delay_press != 0) {
            msg_d = " через "+ String(sec_delay_press, DEC) +" сек. будет";
            delay_press[channel].once(sec_delay_press, Timer_channel_write, (channel<<8) + value);
        } else {
            wfr_channels.write(channel, value);
        }

        value = wfr_channels.read(channel);
        data["value"] = value;

        if (value == 1) {
          if (sec_timer != 0) msg_t = " Включится " + msg_t;
          answer["message"] = "Канал " +String(channel+1, DEC)+ msg_d + " включён!" + msg_t;
        } else {
          if (sec_timer != 0) msg_t = " Выключится " + msg_t;
          answer["message"] = "Канал " +String(channel+1, DEC)+ msg_d + " выключен!" + msg_t;
        }

    } else if (action == "led") {
          
        byte value = webServer.arg("value").toInt();
        digitalWrite(2, value==1?0:1);

        value = digitalRead(2)==1?0:1;
        data["value"] = value;

        if (value == 1) answer["message"] = "LED включён!";
        else answer["message"] = "LED выключен!";
            
    } else if (action == "settings_mode") {

        String mode = webServer.arg("wifi_mode");
        if (mode.toInt() > 2 ||mode.toInt() < 0) mode = "0";
        ee_data.wifi_mode = mode.toInt();

        EEPROM.put(ee_addr_start_settings, ee_data);
        EEPROM.commit();

        data["value"] = ee_data.wifi_mode;
        answer["message"] = "Сохранено!";

    } else if (action == "settings_device_name") {

        String _device_name = webServer.arg("device_name");
        _device_name.toCharArray(ee_data.device_name, sizeof(ee_data.device_name));

        EEPROM.put(ee_addr_start_settings, ee_data);
        EEPROM.commit();

        answer["message"] = "Сохранено!";

    } else if (action == "settings_other") {

        String _update_time = webServer.arg("update_time");
        ee_data.update_time = _update_time.toInt();

        EEPROM.put(ee_addr_start_settings, ee_data);
        EEPROM.commit();

        answer["message"] = "Сохранено!";

    } else if (action == "settings") {

        String _ssid = webServer.arg("ssidAP");
        String _password = webServer.arg("passwordAP");
        String _ssidAP = webServer.arg("ssid");
        String _passwordAP = webServer.arg("password");
        _ssid.toCharArray(ee_data.ssidAP, sizeof(ee_data.ssidAP));
        _password.toCharArray(ee_data.passwordAP, sizeof(ee_data.passwordAP));
        _ssidAP.toCharArray(ee_data.ssid, sizeof(ee_data.ssid));
        _passwordAP.toCharArray(ee_data.password, sizeof(ee_data.password));

        EEPROM.put(ee_addr_start_settings, ee_data);
        EEPROM.commit();

        answer["message"] = "Сохранено!";

    } else if (action == "settings_time") {

        String _date = webServer.arg("date");
        String _time = webServer.arg("time");
        String source = webServer.arg("source");

        if (source == "ntp") {
        } else if (source == "hand" || source == "browser") {

            //setSyncProvider(RTC.get);   // получаем время с RTC
            //if (timeStatus() != timeSet)
            //    Serial.println("Unable to sync with the RTC"); //синхронизация не удаласть
            //else
            //    Serial.println("RTC has set the system time");
            TimeElements te;
            te.Second = source=="browser" ? webServer.arg("seconds").toInt() : 0;
            te.Minute = _time.substring(3, 5).toInt();
            te.Hour = _time.substring(0, 2).toInt();
            te.Day = _date.substring(8, 10).toInt();
            te.Month = _date.substring(5, 7).toInt();
            te.Year = _date.substring(0, 4).toInt() - 1970; //год в библиотеке отсчитывается с 1970
            time_t timeVal = makeTime(te);
            RTC.set(timeVal);
            setTime(timeVal);
        }

        answer["message"] = "Время обновлено!";

    } else if (action == "get_data") {

        String data_type = webServer.arg("data_type");

        if (data_type == "std" || data_type == "all") {

            data["count_outlets"] = COUNT_OUTLETS;

            JsonObject& _stat = data.createNestedObject("stat");
            statistic_update();
            _stat["vcc"] = stat.vcc;
            _stat["time_h"] = stat.time_h;
            _stat["time_m"] = stat.time_m;
            _stat["time_s"] = stat.time_s;

            _stat["rtc_h"] = stat.rtc_h;
            _stat["rtc_m"] = stat.rtc_m;
            _stat["rtc_s"] = stat.rtc_s;
            _stat["rtc_day"] = stat.rtc_day;
            _stat["rtc_month"] = stat.rtc_month;
            _stat["rtc_year"] = stat.rtc_year;
            _stat["rtc_is"] = stat.rtc_is;
            //data.parseObject();
            //JsonObject& _settings = settings.parseObject(ee_data);

        }
        
        if (data_type == "btn" || data_type == "all") {

            data["bt_panel"] = ee_data.bt_panel;

        }

        if (data_type == "btn" || data_type == "std" || data_type == "all") {

            data["gpio_std"] = wfr_channels.read_all();
            data["gpio_led"] = digitalRead(2)==1?0:1;
            
        }

        if (data_type == "set" || data_type == "all") {

            JsonObject& _settings = data.createNestedObject("settings");
            _settings["wifi_mode"] = ee_data.wifi_mode;
            _settings["password"] = ee_data.password;
            _settings["ssidAP"] = ee_data.ssidAP;
            _settings["passwordAP"] = ee_data.passwordAP;
            _settings["ssid"] = ee_data.ssid;
            _settings["device_name"] = ee_data.device_name;
            _settings["update_time"] = ee_data.update_time;

            JsonObject& _stat = data.createNestedObject("stat");
            statistic_update();

            _stat["rtc_h"] = stat.rtc_h;
            _stat["rtc_m"] = stat.rtc_m;
            _stat["rtc_s"] = stat.rtc_s;
            _stat["rtc_day"] = stat.rtc_day;
            _stat["rtc_month"] = stat.rtc_month;
            _stat["rtc_year"] = stat.rtc_year;
            _stat["rtc_is"] = stat.rtc_is;

        }
        data["update_time"] = ee_data.update_time; // для того, чтобы изменение этого значения сразу вступили в силу

        answer["message"] = "Информация на странице обновлена";

    } else if (action == "bt_panel_save") {

        webServer.arg("bt_panel").toCharArray(ee_data.bt_panel, sizeof(ee_data.bt_panel));

        EEPROM.put(ee_addr_start_settings, ee_data);
        EEPROM.commit();

        //bt_panel = webServer.arg("bt_panel");
        //webServer.arg("bt_panel").toCharArray(bt_panel, sizeof(bt_panel));
        //EEPROM.put(ee_addr_start_bt_panel, bt_panel);

        answer["message"] = "Панель сохранена";

    } else if (action == "settings_reboot") {
        restart();
    } else if (action == "settings_reset") {
        reset_settings();
    } else {
        answer["success"] = 0;
        answer["message"] = "неверный API";
    }

    #if INTERFACE_TYPE == 0

    #elif INTERFACE_TYPE == 1

    #endif

    String sAnswer;
    answer.printTo(sAnswer);
    webServer.send(200, "text/html", sAnswer);
}

void restart() {
    EEPROM.get(ee_addr_start_settings, ee_data);
    WiFi.disconnect(true);
    WiFi.softAPdisconnect(true);
    ESP.restart();
}

void reset_settings() {

    // мигаем светодиодом
    byte y = 0;
    for(byte x = 0; x < 10; x++) {
        y = ~y;
        digitalWrite(2, y);
        delay(250);
    }

    EEPROM.write(ee_addr_start_firstrun, 0);
    EEPROM.commit();
    
    restart();

}

// сброс настроек по нажатию на кнопку
void reset_settings_btn() {
    for(byte x = 0; x < 50; x++) {
        delay(100);
        if (digitalRead(pin_btn_reset)==0) return;
    }
    // Настройки сбросятся только если кнопка была зажата в тнчение 5-ти секунд
    reset_settings();
}

/*
 * argument byte address - 7 bit, without RW bit.
 * argument int value - for PCF8575 - two bytes (for ports P00-P07 and P10-P17)
 */
/*void _i2c_channel_write(byte address, byte value[]) {
  Wire.beginTransmission(address);
  Wire.write(value, 2);
  Wire.endTransmission();
}*/

byte wfr_wifiClient_start(byte trying_count) {
    WiFi.begin(ee_data.ssid, ee_data.password);

    byte x = 0;
    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
        x += 1;
        if (x == trying_count) break;
    }

    if (WiFi.status() != WL_CONNECTED) return 0;
    else return 1;
}

int scheduleList[2][7] = {{-1, -1, -1, -1, -1, 5, 0}, {-1, -1, -1, -1, -1, 10, 1}};

void scheduleRunner(void) {

    TimeElements te;
    time_t timeVal = makeTime(te);
    //RTC.set(timeVal);
    setTime(timeVal);
    
    setSyncProvider(RTC.get);   // получаем время с RTC
    if (timeStatus() != timeSet) setSyncProvider(RTC.get);

    byte _h = hour();
    byte _m = minute();
    byte _s = second();
    byte _day = day();
    byte _month = month();
    int _year = year();

    int do_task;

    for (int i = 0; i < 2; i++) {
        do_task = 0;
        if (scheduleList[i][0] == _year) do_task = 1;
        if (scheduleList[i][1] == _month) do_task = 1;
        if (scheduleList[i][2] == _day) do_task = 1;
        if (scheduleList[i][3] == _h) do_task = 1;
        if (scheduleList[i][4] == _m) do_task = 1;
        if (scheduleList[i][5] == _s) do_task = 1;

        if (do_task == 1) {
            digitalWrite(2, !(scheduleList[i][6]));
        }
    }
}

void setup() {

    /* Первичная инициализация */
  
    Serial.begin(115200);
    delay(10);
    EEPROM.begin(4096);//EEPROM.begin(ee_addr_start_settings+sizeof(ee_data));
    delay(10);
    pinMode(pin_btn_reset, INPUT);
    pinMode(2, OUTPUT); digitalWrite(2, LOW);
//EEPROM.put(ee_addr_start_firstrun, 1); EEPROM.commit(); return;
    Serial.println();
    wfr_channels.init(pin_shiftr_clock, pin_shiftr_latch, pin_shiftr_data, ee_addr_start_channels);
    Serial.println();

    //Wire.begin(pin_i2c_sda, pin_i2c_scl);

    pinMode(pin_btn_reset, INPUT);
    attachInterrupt(pin_btn_reset, reset_settings_btn, RISING);

    /* Инициализация настроек */

    if (EEPROM.read(ee_addr_start_firstrun) != code_firstrun) { // Устанавливаем настройки по умолчанию (если изделие запущено первый раз или настройки быв сброшены пользователем)
        EEPROM.put(ee_addr_start_settings, ee_data);
        EEPROM.put(ee_addr_start_channels, 0);
        EEPROM.write(ee_addr_start_firstrun, code_firstrun); // при презапуске устройства код в этих скобках уже не выполнится, если вновь не сбросить натсройки
        EEPROM.commit();
    }

    EEPROM.get(ee_addr_start_settings, ee_data);

    /* подготовка к запуску wifi */

    Serial.println("");
    byte _wifi_mode = ee_data.wifi_mode; // Не трогаем исходное значение

    /* WiFi как клиент */

    if (_wifi_mode == 1 || _wifi_mode == 2) {

        /*WiFi.begin(ee_data.ssid, ee_data.password);

        byte x = 0;
        while (WiFi.status() != WL_CONNECTED) {
            delay(500);
            Serial.print(".");
            x += 1;
            if (x == 20) break;
        }*/

        is_wifi_client_connected = wfr_wifiClient_start(20);
    
        if (is_wifi_client_connected == 1) {
            Serial.println("WiFi connected");
            Serial.print("Client IP: ");
            Serial.println(WiFi.localIP());
        } else { // если не подключились в качестве клиента, то запускаемся в качестве точки доступа
            WiFi.disconnect(true);
            _wifi_mode = 0;
        }

    }

    /* WiFi как точка доступа */

    if (_wifi_mode == 0 || _wifi_mode == 2) {

        WiFi.softAP(ee_data.ssidAP, ee_data.passwordAP);
      
        Serial.println("AP started");
        Serial.print("AP IP address: ");
        Serial.println(WiFi.softAPIP());

        dnsServer.start(53, "*", IPAddress(192, 168, 4, 1));

    }

    /* Запускаем веб-сервер */

    webServer.on("/", HTTP_GET, handler_index_html);
    webServer.on("/api.c", HTTP_POST, apiHandler);
    set_handlers();

    webServer.onNotFound(notFoundHandler);
    webServer.begin();
    //server.begin();
    Serial.println("Server started");

    /* запускаем выполнение расписания */

    schedule.attach(0.4, scheduleRunner);
}

void loop() {

    dnsServer.processNextRequest();
    webServer.handleClient();

    //if (digitalRead(pin_btn_reset)==1) reset_settings();

    // если мы не смогли подключиться к сети при старте - пробуем ещё
    if (is_wifi_client_connected == 0 && (ee_data.wifi_mode == 1 || ee_data.wifi_mode == 2)) {
        is_wifi_client_connected = wfr_wifiClient_start(8);
        // если мы подключились к сети, но точку доступа включать не планировали, то выключим её
        if (is_wifi_client_connected == 1 && ee_data.wifi_mode == 1) {
            WiFi.softAPdisconnect();
        }
    }

    //digitalWrite(2, LOW);
    //delay(250);
    //digitalWrite(2, HIGH);
    //delay(250);

    /*} else if (api_group == "gpio_i2c") {

        int num = -1, val = -1;
        if (api_name != "") num = api_name.toInt();
        if (api_value != "") val = api_value.toInt();

        if (num != -1 && val != -1) { // Set GPIO
            byte _val[] = {val, val >> 8};
            _i2c_channel_write(num, _val);
            answer_code = "OK";
        } else {
            answer_code = "NO";
        }

        answer_data = String(num) +String(" - ")+ String(val);
        print_output_answer(client, answer_code, answer_data);*/
}

