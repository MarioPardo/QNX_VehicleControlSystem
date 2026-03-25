#include "includes/defs.h"

// parser shouldnt even check the type 
// Parser checks for subsystem 
// subsytem works . 
// parser still has to convert the datatype though , i dont think its wise to just move json files and convert again , might as well convert once and leave the pther fucntions purely for processing 





// Example of data , schema basically 
//  Subsytem : ( Im thinking this meant whwere it belongs essentially )
//  Type :  (This should be the message type as in the fake ecu for reference)
//  Data : ( This contains the message being parsed in could be the messages etc)


// Harun - Will use this in other files to initialize struct - to ensure whatever isnt used is auto set to default

void packet_init(msg_packet *c)
{
    strcpy(c->subsys, "");
    strcpy(c->msg_type, "");
    c->msg.percentage = -1.0;
    c->msg.angle = -1.0;
    c->msg.enabled = 0;  // Evaluates to false
}

// telemetry struct → JSON to send to Python
char *telemetry_to_json(telemetry_packet *t) {
    cJSON *root = cJSON_CreateObject();
    cJSON *data = cJSON_CreateObject();

    
    cJSON_AddStringToObject(root, "type",      t->type);
    

    cJSON_AddNumberToObject(data, "Speed",         t->tel.speed);
    // cJSON_AddNumberToObject(data, "Throttle",      t->tel.throttle);
    // cJSON_AddNumberToObject(data, "Brake",         t->tel.brake);
    // cJSON_AddNumberToObject(data, "SteeringAngle", t->tel.steering_angle);
    // cJSON_AddBoolToObject  (data, "SafeMode",      t->tel.safe_mode);
    // cJSON_AddBoolToObject  (data, "SnowMode",      t->tel.snow_mode);

    cJSON_AddItemToObject(root, "data", data);
    char *out = cJSON_PrintUnformatted(root);
    cJSON_Delete(root);
    return out;
}

// This caters to receiving from both

void json_to_msg_packet(const char *json_str, msg_packet *p) {
    memset(p, 0, sizeof(*p));
    p->msg.percentage = -1.0;
    p->msg.angle      = -1.0;
    p->msg.enabled    = 0;

    cJSON *root = cJSON_Parse(json_str);
    if (!root) {
        fprintf(stderr, "Parse error: %s\n", cJSON_GetErrorPtr());
        return;
    }
    cJSON *origin = cJSON_GetObjectItemCaseSensitive(root, "origin");
    cJSON *type = cJSON_GetObjectItemCaseSensitive(root, "type");
    cJSON *data = cJSON_GetObjectItemCaseSensitive(root, "data");

    if (cJSON_IsString(origin))
        strncpy(p->msg_type, type->valuestring, sizeof(p->msg_type) - 1);

    if (cJSON_IsString(type))
        strncpy(p->msg_type, type->valuestring, sizeof(p->msg_type) - 1);

    if (cJSON_IsObject(data)) {
        // Dashboard incoming inputs / data
        cJSON *percentage = cJSON_GetObjectItemCaseSensitive(data, "Percentage");
        cJSON *angle      = cJSON_GetObjectItemCaseSensitive(data, "Angle");
        cJSON *enabled    = cJSON_GetObjectItemCaseSensitive(data, "Enabled");

        if (cJSON_IsNumber(percentage)) p->msg.percentage = percentage->valuedouble;
        if (cJSON_IsNumber(angle))      p->msg.angle      = angle->valuedouble;
        if (cJSON_IsBool(enabled))      p->msg.enabled    = cJSON_IsTrue(enabled);

        // webots/sim incoming inputs / data
        cJSON *speed = cJSON_GetObjectItemCaseSensitive(data, "speed");
        if (cJSON_IsNumber(speed))      p->msg.speed      = speed->valuedouble;

        //In here it just populates what was present in the data sent , the other fields remain empty or sentinels
        //Omitting fields entirely causes a crash so other fields will be empty , the processes wont be reading them anyways.
    }

    cJSON_Delete(root);
}

/*
New archtype for dtaa handling all data being received into qnx 

    Origin:
    Subsys:
    msg : the data object .. must say wont always be full so the parser has to have a way of skipping over that cant remember if this was implemented or not 

*/

//This fuction is for parsing data being sent to webot sim
char* sim_data_to_json(vehicle_controls v*){
    cJSON *root = cJSON_CreateObject();
    cJSON_AddNumberToObject(root, "throttle_level",  v->throttle_level);
    cJSON_AddNumberToObject(root, "brake_level",  v->brake_level);
    cJSON_AddNumberToObject(root, "steering_level",  v->steering_level);
    cJSON_AddNumberToObject(root, "toggleGear",  v->toggleGear);
    
    char *out = cJSON_PrintUnformatted(root);
    cJSON_Delete(root);
    return out;
}
//Schema being used in ecu py side at the moment TBC 

// Try for each data type as some will be empty given the recent changes whereby the origin will be included in the received message 
// Will have to create a data parser for the data coming in from the webots side and going out of it 

// March 24 work doen in telem last --> Harun Mwondi