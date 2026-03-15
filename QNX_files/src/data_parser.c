#include "includes/defs.h"

// parser shouldnt even check the type 
// Parser checks for subsystem 
// subsytem works . 
// parser still has to convert the datatype though , i dont think its wise to just move json files and convert again , might as well convert once and leave the pther fucntions purely for processing 





// Example of data , schema basically 
//  Subsytem : ( Im thinking this meant whwere it belongs essentially )
//  Type :  (This should be the message type as in the fake ecu for reference)
//  Data : ( This contains the message being parsed in could be the messages etc)


// Harun - Will use this in other files to initialize struct - to ensure whatever isnt used ia auto set to default

void packet_init(msg_packet *c)
{
    strcpy(c->subsys, "");
    strcpy(c->msg_type, "");
    c->msg.percentage = -1.0;
    c->msg.angle = -1.0;
    c->msg.enabled = 0;  // Evaluates to false
}

// i dont thik i need this , im assuming telemetry data will always be there 
// // Harun - Will use to create telemetry packet type 
// void telem_packet_init(telemetry_packet *t){


// }


// converts from  cstruct to json format
char *packet_to_json(msg_packet *p) {
    cJSON *root = cJSON_CreateObject();
    cJSON *data = cJSON_CreateObject();

    cJSON_AddStringToObject(root, "subsys",   p->subsys);
    cJSON_AddStringToObject(root, "msg_type", p->msg_type);

    //For now this is the types ive found but ill add macros so we can include more types later 
    // Id say lets work with this for now as it captures what we need before having more data
    // Upgrde to fields to determine 

    if (p->msg.percentage != -1) cJSON_AddNumberToObject(data, "Percentage", p->msg.percentage);
    if (p->msg.angle      != -1) cJSON_AddNumberToObject(data, "Angle",      p->msg.angle);
    cJSON_AddBoolToObject  (data, "Enabled",    p->msg.enabled);

    cJSON_AddItemToObject(root, "data", data);
    char *out = cJSON_PrintUnformatted(root);
    cJSON_Delete(root);
    return out;
}

//converts from json to struct
void  json_to_packet(const char *json_str , telemetry_packet *t){
    //Haruns- Based on what is being received from dashboard side ATM
    
    //Telemetry data
   
    memset(t, 0, sizeof(*t));  // zero everything safely to start

    cJSON *root = cJSON_Parse(json_str);
    if (!root) {
        fprintf(stderr, "Parse error: %s\n", cJSON_GetErrorPtr());
        return;
    }

    cJSON *type = cJSON_GetObjectItemCaseSensitive(root, "type");
    cJSON *data = cJSON_GetObjectItemCaseSensitive(root, "data");
    cJSON *ts   = cJSON_GetObjectItemCaseSensitive(root, "timestamp");

    if (cJSON_IsString(type))
        strncpy(t->type, type->valuestring, sizeof(t->type) - 1);

    if (cJSON_IsNumber(ts))
        t->timestamp = ts->valuedouble;

    if (cJSON_IsObject(data)) {
        cJSON *speed    = cJSON_GetObjectItemCaseSensitive(data, "Speed");
        cJSON *throttle = cJSON_GetObjectItemCaseSensitive(data, "Throttle");
        cJSON *brake    = cJSON_GetObjectItemCaseSensitive(data, "Brake");
        cJSON *steering = cJSON_GetObjectItemCaseSensitive(data, "SteeringAngle");
        cJSON *safe     = cJSON_GetObjectItemCaseSensitive(data, "SafeMode");
        cJSON *snow     = cJSON_GetObjectItemCaseSensitive(data, "SnowMode");

        if (cJSON_IsNumber(speed))    t->tel.speed          = (float)speed->valuedouble;
        if (cJSON_IsNumber(throttle)) t->tel.throttle        = (float)throttle->valuedouble;
        if (cJSON_IsNumber(brake))    t->tel.brake           = (float)brake->valuedouble;
        if (cJSON_IsNumber(steering)) t->tel.steering_angle  = (float)steering->valuedouble;
        if (cJSON_IsBool(safe))       t->tel.safe_mode       = cJSON_IsTrue(safe);
        if (cJSON_IsBool(snow))       t->tel.snow_mode       = cJSON_IsTrue(snow);
    }

    cJSON_Delete(root);
    


}

//Schema being used in ecu py side at the moment TBC 


