#include "SaveEcoBotData.h"
#include <cassert>
#include <fstream>
int main() {
 using namespace SaveEcoBotData;
 DynamicJsonDocument doc(2048);
 Measurement m;
 Area area; area.latitude=49.4444; area.longitude=32.0598;
 deserializeJson(doc, R"({"sensor_id":12,"latitude":"49.4444","longitude":"32.0598"})");
 assert(considerStation(doc.as<JsonObjectConst>(),m,area) && m.id==12);
 doc["latitude"]="bad"; assert(!considerStation(doc.as<JsonObjectConst>(),m,area));
 doc["latitude"]="48.5"; assert(!considerStation(doc.as<JsonObjectConst>(),m,area));
 Measurement nearby;
 doc["latitude"]=49.5; doc["longitude"]=32.0598;
 area.radiusKm=1; assert(!considerStation(doc.as<JsonObjectConst>(),nearby,area));
 area.radiusKm=10; assert(considerStation(doc.as<JsonObjectConst>(),nearby,area));
 area.latitude=50.45; area.longitude=30.52;
 assert(!considerStation(doc.as<JsonObjectConst>(),nearby,area));
 deserializeJson(doc,R"({"results":[{"country_code":"UA","feature_code":"PPLA","latitude":50.45,"longitude":30.52}]})");
 assert(cityArea(doc.as<JsonVariantConst>(),area) && area.latitude==50.45);
 doc["results"][1].set(doc["results"][0]); assert(!cityArea(doc.as<JsonVariantConst>(),area));
 doc["results"][1]["feature_code"]="AIRP"; assert(cityArea(doc.as<JsonVariantConst>(),area));
 doc["results"].as<JsonArray>().remove(1); doc["results"][0]["country_code"]="US";
 assert(!cityArea(doc.as<JsonVariantConst>(),area));
 deserializeJson(doc,R"({"data":[{"phenomenon":"gamma_nsv_h","value":146,"updated_at_utc":"2026-09-19T12:00:00Z","is_old":0}]})");
 auto now=timestamp("2026-09-19T12:01:00Z");
 assert(reading(doc.as<JsonVariantConst>(),now,m));
 assert(fabs(m.value-.146)<1e-9 && !m.old && m.id==12);
 assert(!strcmp(m.date,"2026-09-19 12:00Z"));
 doc["data"][0]["is_old"]=1; assert(reading(doc.as<JsonVariantConst>(),now,m)&&m.old);
 doc["data"][0]["value"]=nullptr; assert(!reading(doc.as<JsonVariantConst>(),now,m));
 doc["data"][0]["value"]=146; doc["data"][0]["phenomenon"]="aqi_pm25";
 assert(!reading(doc.as<JsonVariantConst>(),now,m));
 doc["data"][0]["phenomenon"]="gamma_nsv_h";
 assert(!reading(doc.as<JsonVariantConst>(),now-3600,m));
 doc["data"][0].remove("updated_at_utc"); assert(!reading(doc.as<JsonVariantConst>(),now,m));
 assert(!timestamp("2026-02-30T12:00:00Z"));
 DynamicJsonDocument pub(8192);
 std::ifstream fixture("test/fixtures/saveecobot_public_22800.json");
 assert(!deserializeJson(pub,fixture));
 area.latitude=49.44452; area.longitude=32.05738; area.radiusKm=25;
 assert(publicReading(pub.as<JsonVariantConst>(),area,now,m));
 assert(m.id==22800 && fabs(m.value-.150)<1e-9 && m.publicSource && !m.old);
 assert(m.captured==1789798320 && m.distanceKm<.3);
 pub["last_data"][0]["is_old"]=true;
 assert(publicReading(pub.as<JsonVariantConst>(),area,now,m) && m.old);
 area.radiusKm=.01; assert(!publicReading(pub.as<JsonVariantConst>(),area,now,m));
 area.radiusKm=25; area.latitude=50.45;
 assert(!publicReading(pub.as<JsonVariantConst>(),area,now,m));
 area.latitude=49.44452;
 pub["last_data"][0]["phenomenon"]="pm25"; assert(!publicReading(pub.as<JsonVariantConst>(),area,now,m));
 pub["last_data"][0]["phenomenon"]="gamma_cpm"; assert(!publicReading(pub.as<JsonVariantConst>(),area,now,m));
 pub["last_data"][0]["phenomenon"]="gamma";
 pub["last_data"][0]["value"]=nullptr; assert(!publicReading(pub.as<JsonVariantConst>(),area,now,m));
 pub["last_data"][0]["value"]=150;
 assert(!publicReading(pub.as<JsonVariantConst>(),area,now-86400,m));
 pub["last_data"][0].remove("updated_at"); assert(!publicReading(pub.as<JsonVariantConst>(),area,now,m));

}
