/*
   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/


#include <stdio.h>
#include <stdlib.h>
#include <string.h>


/* Attributes State Machine */
enum
{
    TACO_IDX_SVC,

    TACO_IDX_CHAR_READ_DOCS,
    TACO_IDX_CHAR_VAL_READ_DOCS,
    
    TACO_IDX_CHAR_READ_FLAG,
    TACO_IDX_CHAR_VAL_READ_FLAG,
    
    TACO_IDX_CHAR_WRITE_WARP,
    TACO_IDX_CHAR_VAL_WRITE_WARP,

    //todo: HRS in here is a cut/paste job... get rid of it 
    TACO_IDX_NB,
};
//TODO generate flag name
void app_main();
