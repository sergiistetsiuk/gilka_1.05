#include "UkraineAlarmData.h"
#include <cassert>
int main() {
 DynamicJsonDocument doc(4096);
 char region[24]={};
 deserializeJson(doc,R"({"states":[{"regionId":"test","regionType":"State","regionName":"Черкаська область"}]})");
 assert(!UkraineAlarmData::regionId(doc.as<JsonVariantConst>(),"Черкаська область",region,sizeof(region)));
 doc["states"][0]["regionId"]="99";
 assert(UkraineAlarmData::regionId(doc.as<JsonVariantConst>(),"Черкаська область",region,sizeof(region)) && !strcmp(region,"99"));
 assert(!UkraineAlarmData::regionId(doc.as<JsonVariantConst>(),"Other",region,sizeof(region)));
 UkraineAlarmData::Result result;
 deserializeJson(doc,R"([{"regionId":"99","activeAlerts":[]}])");
 assert(UkraineAlarmData::alerts(doc.as<JsonVariantConst>(),"99",result) && result.state=='N');
 assert(!UkraineAlarmData::alerts(doc.as<JsonVariantConst>(),"88",result));
 doc[0]["activeAlerts"][0]["type"]="AIR";
 assert(UkraineAlarmData::alerts(doc.as<JsonVariantConst>(),"99",result) && result.state=='A');
 doc[0]["activeAlerts"][1]["type"]="NUCLEAR";
 assert(UkraineAlarmData::alerts(doc.as<JsonVariantConst>(),"99",result) && !strcmp(result.label,"NUCLEAR ALERT"));
 doc[0]["activeAlerts"].as<JsonArray>().remove(1);doc[0]["activeAlerts"][0]["type"]="INFO";
 assert(UkraineAlarmData::alerts(doc.as<JsonVariantConst>(),"99",result) && result.state=='I');
 doc[0]["activeAlerts"][0]["type"]="UNKNOWN";
 assert(!UkraineAlarmData::alerts(doc.as<JsonVariantConst>(),"99",result));
 doc[0]["activeAlerts"]=nullptr; assert(!UkraineAlarmData::alerts(doc.as<JsonVariantConst>(),"99",result));
 doc.clear();doc.to<JsonArray>(); assert(!UkraineAlarmData::alerts(doc.as<JsonVariantConst>(),"99",result));
 deserializeJson(doc,R"({"lastActionIndex":2147483648})");
 int64_t index;assert(UkraineAlarmData::actionIndex(doc.as<JsonVariantConst>(),index) && index==2147483648LL);
 doc["lastActionIndex"]=-1;assert(!UkraineAlarmData::actionIndex(doc.as<JsonVariantConst>(),index));
 doc["lastActionIndex"]="3";assert(!UkraineAlarmData::actionIndex(doc.as<JsonVariantConst>(),index));
 // Verify projection keeps an explicit empty activeAlerts array (clear) vs null (unknown).
 DynamicJsonDocument filter(256); filter[0]["regionId"]=true;filter[0]["activeAlerts"][0]["type"]=true;
 assert(!deserializeJson(doc,R"([{"regionId":"99","activeAlerts":[]}])",DeserializationOption::Filter(filter)));
 assert(UkraineAlarmData::alerts(doc.as<JsonVariantConst>(),"99",result) && result.state=='N');
}
