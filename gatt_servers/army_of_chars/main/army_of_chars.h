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
    ARMY_OF_CHARS_IDX_SVC,

    ARMY_OF_CHARS_IDX_CHAR_READ_DOCS1,
    ARMY_OF_CHARS_IDX_CHAR_VAL_READ_DOCS1,
    ARMY_OF_CHARS_IDX_CHAR_READ_DOCS2,
    ARMY_OF_CHARS_IDX_CHAR_VAL_READ_DOCS2,
    ARMY_OF_CHARS_IDX_CHAR_READ_DOCS3,
    ARMY_OF_CHARS_IDX_CHAR_VAL_READ_DOCS3,
    ARMY_OF_CHARS_IDX_CHAR_READ_DOCS4,
    ARMY_OF_CHARS_IDX_CHAR_VAL_READ_DOCS4,
    ARMY_OF_CHARS_IDX_CHAR_READ_DOCS5,
    ARMY_OF_CHARS_IDX_CHAR_VAL_READ_DOCS5,
    ARMY_OF_CHARS_IDX_CHAR_READ_DOCS6,
    ARMY_OF_CHARS_IDX_CHAR_VAL_READ_DOCS6,
    ARMY_OF_CHARS_IDX_CHAR_READ_DOCS7,
    ARMY_OF_CHARS_IDX_CHAR_VAL_READ_DOCS7,
    ARMY_OF_CHARS_IDX_CHAR_READ_DOCS8,
    ARMY_OF_CHARS_IDX_CHAR_VAL_READ_DOCS8,
    ARMY_OF_CHARS_IDX_CHAR_READ_DOCS9,
    ARMY_OF_CHARS_IDX_CHAR_VAL_READ_DOCS9,
    ARMY_OF_CHARS_IDX_CHAR_READ_DOCS10,
    ARMY_OF_CHARS_IDX_CHAR_VAL_READ_DOCS10,
    ARMY_OF_CHARS_IDX_CHAR_READ_DOCS11,
    ARMY_OF_CHARS_IDX_CHAR_VAL_READ_DOCS11,
    ARMY_OF_CHARS_IDX_CHAR_READ_DOCS12,
    ARMY_OF_CHARS_IDX_CHAR_VAL_READ_DOCS12,
    ARMY_OF_CHARS_IDX_CHAR_READ_DOCS13,
    ARMY_OF_CHARS_IDX_CHAR_VAL_READ_DOCS13,
    ARMY_OF_CHARS_IDX_CHAR_READ_DOCS14,
    ARMY_OF_CHARS_IDX_CHAR_VAL_READ_DOCS14,
    ARMY_OF_CHARS_IDX_CHAR_READ_DOCS15,
    ARMY_OF_CHARS_IDX_CHAR_VAL_READ_DOCS15,
    ARMY_OF_CHARS_IDX_CHAR_READ_DOCS16,
    ARMY_OF_CHARS_IDX_CHAR_VAL_READ_DOCS16,
    ARMY_OF_CHARS_IDX_CHAR_READ_DOCS17,
    ARMY_OF_CHARS_IDX_CHAR_VAL_READ_DOCS17,
    ARMY_OF_CHARS_IDX_CHAR_READ_DOCS18,
    ARMY_OF_CHARS_IDX_CHAR_VAL_READ_DOCS18,
    ARMY_OF_CHARS_IDX_CHAR_READ_DOCS19,
    ARMY_OF_CHARS_IDX_CHAR_VAL_READ_DOCS19,
    ARMY_OF_CHARS_IDX_CHAR_READ_DOCS20,
    ARMY_OF_CHARS_IDX_CHAR_VAL_READ_DOCS20,
    ARMY_OF_CHARS_IDX_CHAR_READ_DOCS21,
    ARMY_OF_CHARS_IDX_CHAR_VAL_READ_DOCS21,
    ARMY_OF_CHARS_IDX_CHAR_READ_DOCS22,
    ARMY_OF_CHARS_IDX_CHAR_VAL_READ_DOCS22,
    ARMY_OF_CHARS_IDX_CHAR_READ_DOCS23,
    ARMY_OF_CHARS_IDX_CHAR_VAL_READ_DOCS23,
    ARMY_OF_CHARS_IDX_CHAR_READ_DOCS24,
    ARMY_OF_CHARS_IDX_CHAR_VAL_READ_DOCS24,
    ARMY_OF_CHARS_IDX_CHAR_READ_DOCS25,
    ARMY_OF_CHARS_IDX_CHAR_VAL_READ_DOCS25,
    ARMY_OF_CHARS_IDX_CHAR_READ_DOCS26,
    ARMY_OF_CHARS_IDX_CHAR_VAL_READ_DOCS26,
    ARMY_OF_CHARS_IDX_CHAR_READ_DOCS27,
    ARMY_OF_CHARS_IDX_CHAR_VAL_READ_DOCS27,
    ARMY_OF_CHARS_IDX_CHAR_READ_DOCS28,
    ARMY_OF_CHARS_IDX_CHAR_VAL_READ_DOCS28,
    ARMY_OF_CHARS_IDX_CHAR_READ_DOCS29,
    ARMY_OF_CHARS_IDX_CHAR_VAL_READ_DOCS29,
    ARMY_OF_CHARS_IDX_CHAR_READ_DOCS30,
    ARMY_OF_CHARS_IDX_CHAR_VAL_READ_DOCS30,
    ARMY_OF_CHARS_IDX_CHAR_READ_DOCS31,
    ARMY_OF_CHARS_IDX_CHAR_VAL_READ_DOCS31,
    ARMY_OF_CHARS_IDX_CHAR_READ_DOCS32,
    ARMY_OF_CHARS_IDX_CHAR_VAL_READ_DOCS32,
    ARMY_OF_CHARS_IDX_CHAR_READ_DOCS33,
    ARMY_OF_CHARS_IDX_CHAR_VAL_READ_DOCS33,
    ARMY_OF_CHARS_IDX_CHAR_READ_DOCS34,
    ARMY_OF_CHARS_IDX_CHAR_VAL_READ_DOCS34,
    ARMY_OF_CHARS_IDX_CHAR_READ_DOCS35,
    ARMY_OF_CHARS_IDX_CHAR_VAL_READ_DOCS35,
    ARMY_OF_CHARS_IDX_CHAR_READ_DOCS36,
    ARMY_OF_CHARS_IDX_CHAR_VAL_READ_DOCS36,
    ARMY_OF_CHARS_IDX_CHAR_READ_DOCS37,
    ARMY_OF_CHARS_IDX_CHAR_VAL_READ_DOCS37,
    ARMY_OF_CHARS_IDX_CHAR_READ_DOCS38,
    ARMY_OF_CHARS_IDX_CHAR_VAL_READ_DOCS38,
    ARMY_OF_CHARS_IDX_CHAR_READ_DOCS39,
    ARMY_OF_CHARS_IDX_CHAR_VAL_READ_DOCS39,
    ARMY_OF_CHARS_IDX_CHAR_READ_DOCS40,
    ARMY_OF_CHARS_IDX_CHAR_VAL_READ_DOCS40,
    ARMY_OF_CHARS_IDX_CHAR_READ_DOCS41,
    ARMY_OF_CHARS_IDX_CHAR_VAL_READ_DOCS41,
    ARMY_OF_CHARS_IDX_CHAR_READ_DOCS42,
    ARMY_OF_CHARS_IDX_CHAR_VAL_READ_DOCS42,
    ARMY_OF_CHARS_IDX_CHAR_READ_DOCS43,
    ARMY_OF_CHARS_IDX_CHAR_VAL_READ_DOCS43,
    ARMY_OF_CHARS_IDX_CHAR_READ_DOCS44,
    ARMY_OF_CHARS_IDX_CHAR_VAL_READ_DOCS44,
    ARMY_OF_CHARS_IDX_CHAR_READ_DOCS45,
    ARMY_OF_CHARS_IDX_CHAR_VAL_READ_DOCS45,
    ARMY_OF_CHARS_IDX_CHAR_READ_DOCS46,
    ARMY_OF_CHARS_IDX_CHAR_VAL_READ_DOCS46,
    ARMY_OF_CHARS_IDX_CHAR_READ_DOCS47,
    ARMY_OF_CHARS_IDX_CHAR_VAL_READ_DOCS47,
    ARMY_OF_CHARS_IDX_CHAR_READ_DOCS48,
    ARMY_OF_CHARS_IDX_CHAR_VAL_READ_DOCS48,
    ARMY_OF_CHARS_IDX_CHAR_READ_DOCS49,
    ARMY_OF_CHARS_IDX_CHAR_VAL_READ_DOCS49,
    ARMY_OF_CHARS_IDX_CHAR_READ_DOCS50,
    ARMY_OF_CHARS_IDX_CHAR_VAL_READ_DOCS50,
    ARMY_OF_CHARS_IDX_CHAR_READ_DOCS51,
    ARMY_OF_CHARS_IDX_CHAR_VAL_READ_DOCS51,




    ARMY_OF_CHARS_IDX_CHAR_READ_FLAG,
    ARMY_OF_CHARS_IDX_CHAR_VAL_READ_FLAG,
    
    ARMY_OF_CHARS_IDX_CHAR_WRITE_WARP,
    ARMY_OF_CHARS_IDX_CHAR_VAL_WRITE_WARP,

    //todo: HRS in here is a cut/paste job... get rid of it 
    ARMY_OF_CHARS_IDX_NB,
};
//TODO generate flag name
void app_main();
