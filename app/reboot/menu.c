// SPDX-License-Identifier: GPL-2.0+
// © 2019 Mis012

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
bool p = false;
//FUGLY
#define TIMEOUT_TEXT "press volume down for boot menu"
#define TIMEOUT_TEXT_SCALE 2

extern uint32_t target_volume_down(); //used in non-FUGLY code as well; commented out there, will use this

extern struct global_config global_config;
extern bool FUGLY_boot_to_default_entry;

static void handle_timeout() {
	int i;
	int num_iters = global_config.timeout * 1000 / 100; // times 1000 - sec to msec; divided by 100 - see "lower cpu stress"

	fbcon_draw_text(20, 20, TIMEOUT_TEXT, TIMEOUT_TEXT_SCALE, global_config.fcolor);

	for (i = 0; i < num_iters; i++) {
		if (target_volume_down()) {
			fbcon_draw_text(20, 20, TIMEOUT_TEXT, TIMEOUT_TEXT_SCALE, global_config.fcolor);
			return; //continue to boot menu
		}
		thread_sleep(100); //lower cpu stress
	}

	boot_to_entry(global_config.default_entry);
	dprintf(CRITICAL, "ERROR: Booting default entry failed. Forcibly bringing up menu.\n");
}

void FUGLY_default_boot_function() 
{
	if(global_config.timeout == 0) {
		boot_to_entry(global_config.default_entry);
		dprintf(CRITICAL, "ERROR: Booting default entry failed. Forcibly bringing up menu.\n");
	}
	else {
		handle_timeout();
	}
}
// end of FUGLY
void boot_from_mmc(void);
void boot_recovery_from_mmc(void);
#define HARDCODED_ENTRY_COUNT 1
struct hardcoded_entry hardcoded_entry_list[HARDCODED_ENTRY_COUNT] = {
	{.title = "'recovery' partition", .function = boot_recovery_from_mmc},
};

#define BOOT_ENTRY_SCALE 6
#define ACTUAL_FONT_WIDTH (FONT_WIDTH * BOOT_ENTRY_SCALE)
#define ACTUAL_FONT_HEIGHT (FONT_HEIGHT * BOOT_ENTRY_SCALE)

int selected_option = 0;
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
        printf("Clicked: %s\n", lv_list_get_btn_text(obj));
    }
}
static void draw_menu(void) {
//thread_sleep(1000);
//	num_of_boot_entries = get_entry_count();

//fbcon_draw_filled_rectangle(0, 0, 1000, 1000, 0xFFFFFF );
//fbcon_flush();
}

#define KEY_DETECT_FREQUENCY		50

extern int target_volume_up();
//extern uint32_t target_volume_down(); //declared up top because FUGLY

static bool handle_keys(lv_obj_t * list) {
	uint32_t max_width = fbcon_get_width() - 1;
	uint32_t max_height = fbcon_get_height() - 1;
	uint32_t volume_up_pressed = target_volume_up();
	uint32_t volume_down_pressed = target_volume_down();
	uint32_t power_button_pressed = pm8x41_get_pwrkey_is_pressed();

	//FUGLY
	if(FUGLY_boot_to_default_entry) {
		FUGLY_boot_to_default_entry = 0; //in case we interrupt the autoboot
		FUGLY_default_boot_function();
	}
	//end of FUGLY

	if(volume_up_pressed) {
lv_list_up(list);
		if(selected_option > 0)

			selected_option--;
		return 1;
	}
	
	if (volume_down_pressed) {
lv_list_down(list);
		if(selected_option < (num_of_boot_entries + HARDCODED_ENTRY_COUNT - 1))
		selected_option++;
		return 1;
	}

	if(power_button_pressed) {
		if(selected_option < num_of_boot_entries) {
			struct boot_entry *entry = entry_list + selected_option;
			fbcon_draw_filled_rectangle(0, 0, max_width, max_height, global_config.bgcolor);
			char *result = malloc(strlen("Booting ") + strlen(entry->title) + 1); 
    		strcpy(result, "Booting ");
    		strcat(result, entry->title);
			fbcon_draw_text(0, max_height/2, result, BOOT_ENTRY_SCALE, global_config.fcolor);
			boot_to_entry(entry);
		}
		else {
			hardcoded_entry_list[selected_option - num_of_boot_entries].function();
		}
	}

	return 0;
}
bool keyboard_read(lv_indev_drv_t * drv, lv_indev_data_t*data){
	uint32_t volume_up_pressed = target_volume_up();
	uint32_t volume_down_pressed = target_volume_down();
	uint32_t power_button_pressed = pm8x41_get_pwrkey_is_pressed();
    if(volume_up_pressed){
        up=true;
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
    }
  return false; /*No buffering now so no more data read*/
}

int menu_thread(void *arg) {
    num_of_boot_entries = get_entry_count();
	entry_list = (struct boot_entry *)arg;

	fbcon_clear();
	//draw_menu();
    lv_init();
  
    static lv_disp_buf_t disp_buf;
    static lv_color_t buf[LV_HOR_RES_MAX * 10]; /*Declare a buffer for 10 lines*/
    lv_disp_buf_init( & disp_buf, buf, NULL, LV_HOR_RES_MAX * 10); /*Initialize the display buffer*/
    lv_disp_drv_t disp_drv; /*Descriptor of a display driver*/
    lv_disp_drv_init( & disp_drv); /*Basic initialization*/
    disp_drv.flush_cb = my_disp_flush; /*Set your driver function*/
    disp_drv.buffer = & disp_buf; /*Assign the buffer to the display*/
    lv_disp_drv_register( & disp_drv); /*Finally register the driver*/
    thread_resume(thread_create("sleeper", &sleep_thread, NULL, DEFAULT_PRIORITY, DEFAULT_STACK_SIZE));
    lv_obj_t * win = lv_win_create(lv_scr_act(), NULL);
    lv_win_set_title(win, "Boot menu"); 


 /*Create a list*/
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


    lv_obj_set_size(list1, 1080, 1800);
    lv_obj_align(list1, NULL, LV_ALIGN_IN_TOP_LEFT, 0, 100);
//Add list buttons
    int ret;
	struct boot_entry *entry_list = NULL;
	ret = parse_boot_entries(&entry_list);
	if (ret < 0) {
		printf("falied to parse boot entries: %d\n", ret);
		return;
	}
    lv_obj_t * list_btn;
    int i =0;
	for (i = 0; i < num_of_boot_entries; i++) {
		list_btn = lv_list_add_btn(list1, LV_SYMBOL_FILE,(entry_list + i)->title);
        lv_obj_set_event_cb(list_btn, event_handler);
	}
}

// hardcoded functions

extern unsigned int boot_into_recovery;

void boot_from_mmc(void) {
	boot_into_recovery = 0;
	boot_linux_from_mmc();
}

void boot_recovery_from_mmc(void) {
	boot_into_recovery = 1;
	boot_linux_from_mmc();
}

