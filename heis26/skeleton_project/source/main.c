#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <time.h>
#include "driver/elevio.h"
#include <unistd.h>


// To lister for bestillinger i retning hendholdsvis opp og ned.
int Bestilling_opp[4] = {};
int Bestilling_ned[4] = {};
int Siste_Etasje = 0;
int Retning = 0;
int Liste ; 
int Stopp_etasje ;
int Forrige_retning ;

// Initialisering
void Initaliser(){
    elevio_init();
    
    // Setter startposisjon
    int Start_pos = elevio_floorSensor();
    while (Start_pos != 0){
        elevio_motorDirection(DIRN_DOWN);
        Start_pos = elevio_floorSensor();
    }
    Siste_Etasje = 0;
    elevio_motorDirection(DIRN_STOP);
    elevio_floorIndicator(Siste_Etasje);

    for (int i = 0; i < 4; i++){
        for (int b = 0; b < 3; b++){
            elevio_buttonLamp(i, b, 0);
        }
    }
    elevio_stopLamp(0);
}

// Stopp knapp implementsjon
int Stoppknapp(){
        int Stopp_knapp = elevio_stopButton();

        while (Stopp_knapp == 1){
            elevio_stopLamp(1);
            elevio_motorDirection(DIRN_STOP);
            Stopp_knapp = elevio_stopButton();
            int Etasje_sensor = elevio_floorSensor();
            
            // Hvis stoppknapp presses i etasje
            if (Etasje_sensor >= 0){
                elevio_doorOpenLamp(1);
                sleep(3);
                elevio_doorOpenLamp(0);
            }

            // Slette bestillinger
            if (Stopp_knapp == 0){
                for (int i = 0; i < 4; i++) {
                    Bestilling_ned[i] = 0;
                    Bestilling_opp[i] = 0;

                    // Skru av alle lys hvis stoppknapp er presset
                    for (int b = 0; b < 3; b ++){
                        elevio_buttonLamp(i, b, 0);
                    }
                
                }
                Forrige_retning = Retning;
                Retning = 0;
                elevio_stopLamp(0);
                return 1;
            }
        
        }
        return 0;
}

// Siste etasje oppdatering
void Oppdater_etasje(){
    int Sensor_etasje = elevio_floorSensor();
    if (Sensor_etasje != -1){
        Siste_Etasje = Sensor_etasje;
        elevio_floorIndicator(Sensor_etasje);
    }
}

// Sjekker bestillingsknapper
void Oppdater_bestillinger(){
    for (int floor = 0; floor < 4; floor++){
        for (int Button = 0; Button < 3; Button++){
            int Bestilling_knapp = elevio_callButton(floor, Button);
            Stoppknapp();

            // Sjekker om en bestillingsknapp er trykket
            if (Bestilling_knapp == 1){
                elevio_buttonLamp(floor, Button, 1);
                if (Button == 0){
                    Bestilling_opp[floor] = 1;
                }

                if (Button == 1){
                    Bestilling_ned[floor] = 1;
                }

                if (Button == 2){
                    Bestilling_ned[floor] = 1;
                    Bestilling_opp[floor] = 1;
                }    
            }
            
        }
    }
}

// Stopp i etasje
int Stoppet_i_etasje(){
    elevio_motorDirection(0);
    elevio_doorOpenLamp(1);
    elevio_floorIndicator(Siste_Etasje);
    time_t start = time(NULL);

    // Venter 3 sekunder
    while(difftime(time(NULL), start) < 3){
        Oppdater_bestillinger();
        Stoppknapp();
    }

    // Venter hvis obstrusjon
    int obstruksjon = elevio_obstruction();
    if (obstruksjon == 1){
        while(obstruksjon ==1){
            obstruksjon = elevio_obstruction();
        }
        time_t start_obstruksjon = time(NULL);
        while(difftime(time(NULL), start_obstruksjon) < 3){
            Oppdater_bestillinger();
            Stoppknapp();
    }    
    }
    elevio_doorOpenLamp(0);

    // Slette bestilling som du oppfyller
    Bestilling_ned[Siste_Etasje] = 0;
    Bestilling_opp[Siste_Etasje] = 0;
    elevio_buttonLamp(Siste_Etasje, 2, 0);
    elevio_buttonLamp(Siste_Etasje, 1, 0);
    elevio_buttonLamp(Siste_Etasje, 0, 0);

    if (Liste == 1){
        for (int i = Siste_Etasje; i < 4; i++){
            if (Bestilling_opp[i] == 1) {
                return Retning;
            }
        }
        return 0;
    }

    if (Liste == -1){
        for (int i = Siste_Etasje; i >= 0; i--){
            if (Bestilling_ned[i] == 1) {
                return Retning;
            }
        }
        return 0;
    }
    return 0;
}

// Sjekker bestillingslister
int Sjekk_lister(){

    // Returnerer retningen man kjører og hvilken liste man ser på
    for (int etasje = 0; etasje < 4; etasje++){
        if(Bestilling_opp[etasje] == 1 && Siste_Etasje < etasje){
            Liste = 1;
            return 1;
        }
        if(Bestilling_opp[etasje] == 1 && Siste_Etasje > etasje){
            Liste = 1;
            return -1;
        }
    }


    for (int etasje = 0; etasje < 4; etasje++){
        if(Bestilling_ned[etasje] == 1 && Siste_Etasje > etasje){
            Liste = -1;
            return -1;
        }

        if(Bestilling_ned[etasje] == 1 && Siste_Etasje < etasje){
            Liste = -1;
            return 1;
        }
    }
    return 0;
}

// Finne etasjen man skal stoppe i
int Finn_etasje_stopp(){

    // Er i bestilling opp lista
    if (Liste == 1){
        for (int i = 0; i < 4; i++){
            if (Bestilling_opp[i] == 1){
                if (Retning == -1 || Retning == 0){
                    for (int f = 3; f >= 0; f--){
                        if (Bestilling_ned[f] == 1){
                            return f;
                        }
                    }
                }
                else {
                    return i;
                }
            }
        }
    }



    if (Liste == -1){
        for (int i = 3; i >= 0; i--){
            if (Bestilling_ned[i] == 1){
                if (Retning == 1 || Retning == 0){
                    for (int f = 0; f < 4; f++){
                        if (Bestilling_opp[f] == 1){
                            return f;
                        }
                    }
                }
                else {
                    return i;
                }
            }
        }
    }
    return 10;

}



int main(){
    Initaliser();

    while (1){
        elevio_motorDirection(Retning);
        Oppdater_etasje();
        if (Retning == 0){
            Retning = Sjekk_lister();
        }

        Stopp_etasje = Finn_etasje_stopp();

        // Sjekker om man skal stoppe i etasjen
        if (Liste == 1){
            if (elevio_floorSensor() == Stopp_etasje){
                Retning = Stoppet_i_etasje();

                if (Retning != 0){
                    Retning = 1;
                }
            }  
        }
        if (elevio_floorSensor() == -1 && Stopp_etasje == Siste_Etasje && Retning == Forrige_retning){
            Retning = -1 * Forrige_retning;
        }

        if (Liste == -1){
            if (elevio_floorSensor() == Stopp_etasje){
                Retning = Stoppet_i_etasje();

                if (Retning != 0){
                    Retning = -1;
                }
            }    
        }
        Oppdater_bestillinger();
        Stoppknapp();
        
        elevio_motorDirection(Retning);

        

        // Stoppe ved grensene
        if (Siste_Etasje == 3 && Retning == 1){
            Retning = 0;
        }
        if (Siste_Etasje == 0 && Retning == -1){
            Retning = 0;
        }

        int avbryt = elevio_obstruction();
        if (avbryt == 1){
            return 0;
        }
        
    }
    
    return 0;
}

