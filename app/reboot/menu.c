// SPDX-License-Identifier: GPL-2.0+
// © 2019 Mis012
// © 2020 luka177

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <pm8x41.h>
#include <pm8x41_hw.h>
#include <kernel/thread.h>
#include <dev/fbcon.h>
#include <target.h>
#include <lib/lvgl.h>
#include "aboot/aboot.h"

#include "boot.h"
#include "config.h"

int num_of_boot_entries;

struct boot_entry *entry_list;

struct hardcoded_entry {
	char *title;
	void (*function)(void);
};
bool up = false;
bool down = false;
bool enter = false;
bool p = false;

extern uint32_t target_volume_down(); //used in non-FUGLY code as well; commented out there, will use this

extern struct global_config global_config;

void boot_from_mmc(void);
void boot_recovery_from_mmc(void);
#define HARDCODED_ENTRY_COUNT 1
struct hardcoded_entry hardcoded_entry_list[HARDCODED_ENTRY_COUNT] = {
	{.title = "'recovery' partition", .function = boot_recovery_from_mmc},
};


static int sleep_thread(void * arg) {
  /*Handle LitlevGL tasks (tickless mode)*/
  while (1) {
    lv_tick_inc(5);
    lv_task_handler();
    thread_sleep(5);
  }

  return 0;
}

void my_disp_flush(lv_disp_t * disp,
  const lv_area_t * area, lv_color_t * color_p) {
  uint x, y;
  for (y = area -> y1; y <= area -> y2; y++) {
    for (x = area -> x1; x <= area -> x2; x++) {
      fbcon_draw_pixel(x, y, 0xff << 24 | color_p->ch.red << 16 | color_p->ch.green << 8 | color_p->ch.blue  ); /* Put a pixel to the display.*/
      color_p++;
    }
  }
  fbcon_flush();
  lv_disp_flush_ready(disp); /* Indicate you are ready with the flushing*/
}

static void event_handler(lv_obj_t * obj, lv_event_t event)
{
    if(event == LV_EVENT_CLICKED) {
        int index = lv_list_get_btn_index(NULL, obj);
        struct boot_entry *entry = entry_list + index;
        boot_to_entry(entry);
    }
}

extern int target_volume_up();

bool keyboard_read(lv_indev_drv_t * drv, lv_indev_data_t*data){
	uint32_t volume_up_pressed = target_volume_up();
	uint32_t volume_down_pressed = target_volume_down();
	uint32_t power_button_pressed = pm8x41_get_pwrkey_is_pressed();
    if(volume_up_pressed){
        up=true;
        p=true;
    }
    if(power_button_pressed){
        enter=true;
        p=true;
    }
    if(volume_down_pressed){
        down=true;
        p=true;
    }
    if(up){
        data->key = LV_KEY_UP;            /*Get the last pressed or released key*/
        printf("up");
    }
    if(enter){
        data->key = LV_KEY_ENTER;            /*Get the last pressed or released key*/
        printf("up");
    }

    if(down){
        data->key = LV_KEY_DOWN;            /*Get the last pressed or released key*/
        printf("down");
    }

    if(p){
        data->state = LV_INDEV_STATE_PR;
        printf("press\n");
        p=false;
    }
    else {
        data->state = LV_INDEV_STATE_REL;
        //printf("release\n");
        up=false;
        down=false;
        enter=false;
    }
  return false; /*No buffering now so no more data read*/
}

int menu_thread(void *arg) {
    //Get entry list and num of boot entries
    num_of_boot_entries = get_entry_count();
	entry_list = (struct boot_entry *)arg;
    
    //Clear screen and init LVGL
	fbcon_clear();
    lv_init();
    fbcon_clear();
thread_sleep(300);
    //Set up buffer and init screen
    static lv_disp_buf_t disp_buf;
    static lv_color_t buf[LV_HOR_RES_MAX * 10]; /*Declare a buffer for 10 lines*/
    lv_disp_buf_init( & disp_buf, buf, NULL, LV_HOR_RES_MAX * 10); /*Initialize the display buffer*/
    lv_disp_drv_t disp_drv; /*Descriptor of a display driver*/
    lv_disp_drv_init( & disp_drv); /*Basic initialization*/
    disp_drv.flush_cb = my_disp_flush; /*Set your driver function*/
    disp_drv.buffer = & disp_buf; /*Assign the buffer to the display*/
    lv_disp_drv_register( & disp_drv); /*Finally register the driver*/

    //Create thread
    thread_resume(thread_create("sleeper", &sleep_thread, NULL, DEFAULT_PRIORITY, DEFAULT_STACK_SIZE));
    LV_THEME_DEFAULT_INIT(LV_COLOR_GRAY, LV_COLOR_GRAY,
                          LV_THEME_MATERIAL_FLAG_DARK,
                          lv_theme_get_font_small(), lv_theme_get_font_normal(), lv_theme_get_font_subtitle(), lv_theme_get_font_title());
    fbcon_clear();
    thread_sleep(300);
    //Create window
    lv_obj_t * win = lv_win_create(lv_scr_act(), NULL);

    //Set window title
    lv_win_set_title(win, "Boot menu"); 

    //Create list
    lv_obj_t * list1 = lv_list_create(win, NULL);

    //Create group
    lv_group_t * g1 = lv_group_create();
    lv_group_add_obj(g1, list1);
    lv_group_focus_obj(list1);

    //Drivers
    lv_indev_drv_t indev_drv;
    lv_indev_drv_init(&indev_drv);      /*Basic initialization*/
    indev_drv.type = LV_INDEV_TYPE_KEYPAD;
    indev_drv.read_cb = keyboard_read;

    /*Register the driver in LVGL and save the created input device object*/
    lv_indev_t * my_indev = lv_indev_drv_register(&indev_drv);
    lv_indev_set_group(my_indev, g1);

    //Set list size and align
    lv_obj_set_size(list1, 1075, 1750);
    lv_obj_align(list1, NULL, LV_ALIGN_IN_TOP_LEFT, 0, 0);
    lv_win_set_scrollbar_mode(win, LV_SCRLBAR_MODE_OFF);
    //Parse boot entries
    int ret;
	struct boot_entry *entry_list = NULL;
	ret = parse_boot_entries(&entry_list);
	if (ret < 0) {
		printf("falied to parse boot entries: %d\n", ret);
		return;
	}

    //Create list buttons from parsed boot entries
    lv_obj_t * list_btn;
    int i =0;
	for (i = 0; i < num_of_boot_entries; i++) {
		list_btn = lv_list_add_btn(list1, LV_SYMBOL_FILE,(entry_list + i)->title);
        lv_obj_set_event_cb(list_btn, event_handler);
	}

    //Add "Extras" button
    //list_btn = lv_list_add_btn(list1, LV_SYMBOL_FILE,"Extras");
    //lv_obj_set_event_cb(list_btn, event_handler);
    
    //lv_obj_set_event_cb(list_btn, event_handler);
    lv_obj_t * label1;
    int output = 100;
    lv_obj_t *cont;
    /*if (output==0)
        cont=lv_obj_get_parent(lv_win_add_btn(win, LV_SYMBOL_BATTERY_EMPTY));
    if (output < 33 && output>0)
        cont=lv_obj_get_parent(lv_win_add_btn(win, LV_SYMBOL_BATTERY_1));
    if (output >=33 && output<66)
        cont=lv_obj_get_parent(lv_win_add_btn(win, LV_SYMBOL_BATTERY_2));
    if (output >=66 && output<99)
        cont=lv_obj_get_parent(lv_win_add_btn(win, LV_SYMBOL_BATTERY_3));
    if (output ==100) 
        cont=(lv_obj_get_parent(lv_win_add_btn(win, LV_SYMBOL_BATTERY_FULL)));*/
    lv_win_add_btn(win, LV_SYMBOL_BATTERY_FULL);
    //label1 = lv_label_create(cont, NULL);
    //lv_label_set_text_fmt(label1, "%d%%", output);
    //lv_obj_align(label1, NULL, LV_ALIGN_IN_TOP_RIGHT, -150, 55);
   
}
