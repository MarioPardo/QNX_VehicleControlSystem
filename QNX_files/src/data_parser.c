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
// Might not use at the moment
void packet_init(msg_packet *c)
{
    strcpy(c->origin, "");
    strcpy(c->subsys, "");
    c->msg.percentage = -1.0;
    c->msg.angle = -1.0;
    c->msg.enabled = 0;  
    c->msg.speed = -1.0;
}

//Have to use as memset values correspond to expected values in webots
//This just initializes the struct with the default values to symbolize it empty or unchanged . For now im using the value -9999 (OUT OF RANGE)
void vc_init(vehicle_controls *vc){
    strcpy(vc->subsys, "");
    strcpy(vc->data.toggleGear, "");
    vc->data.throttle_level = -9999;
    vc->data.brake_level = -9999;
    vc->data.steering_level = -9999;  
    vc->data.snow_mode = 0;
}


// telemetry struct → JSON to send to Python
char *telemetry_to_json(telemetry_packet *t) {
    cJSON *root     = cJSON_CreateObject();
    cJSON *data     = cJSON_CreateObject();
    cJSON *mode     = cJSON_CreateObject();       
    cJSON *warnings = cJSON_CreateArray();

    cJSON_AddStringToObject(root, "type", t->type);

    cJSON_AddNumberToObject(data, "Speed", t->tel.speed);

    cJSON_AddBoolToObject(mode, "Snow", t->tel.snow_mode);  // ← nested under Mode in dashboard 
    cJSON_AddItemToObject(data, "Mode", mode);

    for (int i = 0; i < t->tel.warning_count; i++)
        cJSON_AddItemToArray(warnings, cJSON_CreateString(t->tel.warnings[i]));
    cJSON_AddItemToObject(data, "Warnings", warnings);

    cJSON_AddItemToObject(root, "data", data);
    char *out = cJSON_PrintUnformatted(root);
    cJSON_Delete(root);
    return out;
}

// This caters to receiving from both dashboard and webots

void json_to_msg_packet(const char *json_str, msg_packet *p) {

    // Default values

    memset(p, 0, sizeof(*p));
   p->msg.percentage   = -999.0;
    p->msg.throttle     = -999.0;
    p->msg.angle        = -999.0;
    p->msg.enabled      = 0;    //false for snowmode
    p->msg.speed        = -999.0;
    p->msg.temp         = -999.0;
    p->msg.chaos_active = 0;
    strncpy(p->msg.toggleGear, "D", sizeof(p->msg.toggleGear) - 1);

    cJSON *root = cJSON_Parse(json_str);
    if (!root) {
        fprintf(stderr, "Parse error: %s\n", cJSON_GetErrorPtr());
        return;
    }
    cJSON *origin = cJSON_GetObjectItemCaseSensitive(root, "origin");
    cJSON *subsys = cJSON_GetObjectItemCaseSensitive(root, "subsys");
    cJSON *data = cJSON_GetObjectItemCaseSensitive(root, "data");

    if (cJSON_IsString(origin)){
        strncpy(p->origin, origin->valuestring, sizeof(p->origin) - 1);
        p->origin[sizeof(p->origin) - 1] = '\0';
    }

    if (cJSON_IsString(subsys)){
        strncpy(p->subsys, subsys->valuestring, sizeof(p->subsys) - 1);
        p->subsys[sizeof(p->subsys) - 1] = '\0';
    }

    if (!cJSON_IsObject(data)) {
        cJSON_Delete(root);
        return;
    }

    
    // ----------------------------------------------------------
    // BRAKE:  { "subsys": "Brake", "data": { "brake": { "level": 0.4 } } }
    // ----------------------------------------------------------
    if (strcmp(p->subsys, "Brake") == 0) {

        cJSON *brake = cJSON_GetObjectItemCaseSensitive(data, "brake");
        if (cJSON_IsObject(brake)) {
            cJSON *level = cJSON_GetObjectItemCaseSensitive(brake, "level");
            if (cJSON_IsNumber(level))
                p->msg.percentage = level->valuedouble;
        }
    }

    // ----------------------------------------------------------
    // DRIVING: { "subsys": "Driving", "data": { "steering": { "angle": 0.2 },
    //                                            "throttle": { "level": 0.6 },
    //                                            "gear": "D" } }
    // ----------------------------------------------------------
    else if (strcmp(p->subsys, "Driving") == 0) {

        cJSON *steering = cJSON_GetObjectItemCaseSensitive(data, "steering");
        if (cJSON_IsObject(steering)) {
            cJSON *angle = cJSON_GetObjectItemCaseSensitive(steering, "angle");
            if (cJSON_IsNumber(angle))
                p->msg.angle = angle->valuedouble;
        }

        cJSON *throttle = cJSON_GetObjectItemCaseSensitive(data, "throttle");
        if (cJSON_IsObject(throttle)) {
            cJSON *level = cJSON_GetObjectItemCaseSensitive(throttle, "level");
            if (cJSON_IsNumber(level))
                p->msg.throttle = level->valuedouble;
        }

        cJSON *gear = cJSON_GetObjectItemCaseSensitive(data, "gear");
        if (cJSON_IsString(gear)) {
            strncpy(p->msg.toggleGear, gear->valuestring, sizeof(p->msg.toggleGear) - 1);
            p->msg.toggleGear[sizeof(p->msg.toggleGear) - 1] = '\0';
        }
    }

    // ----------------------------------------------------------
    // MODE: { "subsys": "Mode", "data": { "snow": true, "chaos": "brake"|null } }
    // ----------------------------------------------------------
    else if (strcmp(p->subsys, "Mode") == 0) {

        cJSON *snow = cJSON_GetObjectItemCaseSensitive(data, "snow");
        if (cJSON_IsBool(snow))
            p->msg.enabled = cJSON_IsTrue(snow);
        else if (cJSON_IsNumber(snow))
            p->msg.enabled = (snow->valueint != 0);

        cJSON *chaos = cJSON_GetObjectItemCaseSensitive(data, "chaos");
        if (cJSON_IsString(chaos) && strlen(chaos->valuestring) > 0)
            p->msg.chaos_active = 1;
        // if null → chaos_active stays 0
    }

    // ----------------------------------------------------------
    // WEBOTS/SIM:  { "origin": "VehicleInput", "data": { "speed": 12.3, "temp": 22.1 } }
    // ----------------------------------------------------------
    else {
        cJSON *speed = cJSON_GetObjectItemCaseSensitive(data, "speed");
        if (cJSON_IsNumber(speed))
            p->msg.speed = speed->valuedouble;

        cJSON *temp = cJSON_GetObjectItemCaseSensitive(data, "temp");
        if (cJSON_IsNumber(temp))
            p->msg.temp = temp->valuedouble;
    }


    cJSON_Delete(root);
}

/*
New archtype for dtaa handling all data being received into qnx 

    Origin:
    Subsys:
    msg : the data object .. must say wont always be full so the parser has to have a way of skipping over that cant remember if this was implemented or not 

*/

//This fuction is for parsing data being sent to -> WEBOT SIM

char* sim_data_to_json(vehicle_controls *v){
    cJSON *root = cJSON_CreateObject();
    cJSON *data = cJSON_CreateObject();

    cJSON_AddStringToObject(root, "subsys",      v->subsys);
    

    //This basically auto populates each field to null or -999 value that way it only parses in the value required
    if (v->data.throttle_level != -999) 
        cJSON_AddNumberToObject(data, "throttle_level", v->data.throttle_level);
    if (v->data.brake_level != -999)    
        cJSON_AddNumberToObject(data, "brake_level",    v->data.brake_level);
    if (v->data.steering_level != -999) 
        cJSON_AddNumberToObject(data, "steering_level", v->data.steering_level);
   
    //snowmode here is being passed as an int repping true/false
    cJSON_AddNumberToObject(data, "snow_mode", v->data.snow_mode);
    
    cJSON_AddStringToObject(data, "toggleGear",     v->data.toggleGear);


    cJSON_AddItemToObject(root, "data", data);
    
    char *out = cJSON_PrintUnformatted(root);
    cJSON_Delete(root);
    return out;
}
//Schema being used in ecu py side at the moment TBC 

// Try for each data type as some will be empty given the recent changes whereby the origin will be included in the received message 
// Will have to create a data parser for the data coming in from the webots side and going out of it 

// March 24 work doen in telem last --> Harun Mwondi

