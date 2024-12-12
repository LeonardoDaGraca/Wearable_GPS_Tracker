#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#define EARTH_RADIUS_KM 6371.0

//function to convert lat or long from "degrees and minutes" to decimal degrees
double convert_to_decimal_degrees(double degrees_minutes, char direction) {
    int degrees = (int)(degrees_minutes / 100);
    double minutes = degrees_minutes - (degrees * 100);
    double decimal_degrees = degrees + (minutes / 60.0);

    if (direction == 'S' || direction == 'W') {
        decimal_degrees = -decimal_degrees;
    }

    return decimal_degrees;
}

//haversine formula that calculates dist between two points on Earth's surface
double haversine_distance(double lat1, double long1, double lat2, double long2) {
    double dlat = (lat2 - lat1) * M_PI / 180.0;
    double dlong = (long2 - long1) * M_PI / 180.0;

    lat1 = lat1 * M_PI / 180.0;
    lat2 = lat2 * M_PI / 180.0;

    double a = sin(dlat / 2) * sin(dlat / 2) +
               cos(lat1) * cos(lat2) * sin(dlong / 2) * sin(dlong / 2);
    double c = 2 * atan2(sqrt(a), sqrt(1 - a));

    return EARTH_RADIUS_KM * c;
}

int main() {
    FILE *file = fopen("gps_log.txt", "r");
    if(!file) {
        perror("Unable to open file");
        return 1;
    }

    char line[256];
    double total_distance = 0.0;
    double prev_lat = 0.0, prev_long = 0.0;
    int prev_time = 0;
    int is_first_valid_entry = 1;

    while (fgets(line, sizeof(line), file)) {
        double time, lat, longt;
        char lat_dir, long_dir;
        int fix;

        //parse line
        int fields_read = sscanf(line, "%lf,%lf,%c,%lf,%c,%d", &time, &lat, &lat_dir, &longt, &long_dir, &fix);
        //check if line has valid data
        if (fields_read < 6 || fix == 0 || lat == 0 || longt == 0) {
            continue;
        }

        double lat_decimal = convert_to_decimal_degrees(lat, lat_dir);
        double long_decimal = convert_to_decimal_degrees(longt, long_dir);

        if(!is_first_valid_entry) {
            //calculate distance from previous point
            double distance = haversine_distance(prev_lat, prev_long, lat_decimal, long_decimal);
            total_distance += distance;
        }

        //update values
        prev_lat = lat_decimal;
        prev_long = long_decimal;
        prev_time = (int)time;
        is_first_valid_entry = 0;
    }
    
    fclose(file);

    printf("Total Distance: %.2f km\n", total_distance);

    return 0;
}
