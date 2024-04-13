/***************************************************************************
 *   Author Alan Crispin                                                   *
 *   crispinalan@gmail.com                                                 *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation.                                         *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program.  If not, see <http://www.gnu.org/licenses/>. *
 ***************************************************************************/

//====================================================================
// GTK4 Talk Calendar
// Author: Alan Crispin <crispinalan@gmail.com> 
// Date: March 2024
// use make file to compile
//====================================================================


#include <gtk/gtk.h>

#include <glib/gstdio.h>  //needed for g_mkdir
#include <math.h>  //compile with -lm
#include <libnotify/notify.h>  //notifications


#include "displayitem.h"
#include "calendarevent.h"
#include "customcalendar.h"
#include "dbmanager.h"

//voice 2024

#include "voice3.h"

#define CONFIG_DIRNAME "talkcal-gtk4-010"
#define CONFIG_FILENAME "talkcal-config-013"

//threads
static GMutex lock;

//Declarations
//File Callbks
static void callbk_export(GSimpleAction *action, GVariant *parameter,  gpointer user_data);
static void callbk_import(GSimpleAction *action, GVariant *parameter,  gpointer user_data);
static void callbk_quit(GSimpleAction* action,G_GNUC_UNUSED GVariant *parameter, gpointer user_data);
void export_csv_file();
void import_csv_file(gpointer user_data);
int file_exists(const char *file_name);

//Edit Callbks
static void callbk_preferences(GSimpleAction* action, GVariant *parameter,gpointer user_data);

//Event callbks

static void callbk_add_event(GtkButton *button, gpointer user_data);
static void callbk_new_event(GSimpleAction *action, GVariant *parameter,  gpointer user_data);
static void callbk_update_event(GtkButton *button, gpointer user_data);
static void callbk_edit_event(GSimpleAction *action, GVariant *parameter,  gpointer user_data);
static void callbk_delete_selected(GSimpleAction *action, GVariant *parameter,  gpointer user_data);
static void callbk_delete_all(GSimpleAction *action, GVariant *parameter,  gpointer user_data);

static void callbk_dropdown(GtkDropDown* self, gpointer user_data);
static void callbk_check_button_allday_toggled (GtkCheckButton *check_button, gpointer user_data);

//calendar menu items
static GMenu *create_menu(const GtkApplication *app);

static void callbk_calendar_home(GSimpleAction * action, GVariant *parameter, gpointer user_data);
static void callbk_update_colours(GtkButton *button, gpointer user_data);
static void callbk_calendar_colour(GSimpleAction * action, GVariant *parameter, gpointer user_data);
static void callbk_calendar_speaktime(GSimpleAction * action, GVariant *parameter, gpointer user_data);
gboolean is_colour_name(gchar* colour_name);

//calendar callbks
static void callbk_calendar_day_selected(CustomCalendar *calendar, gpointer user_data);
static void callbk_calendar_next_month(CustomCalendar *calendar, gpointer user_data);
static void callbk_calendar_prev_month(CustomCalendar *calendar, gpointer user_data);
static void callbk_calendar_next_year(CustomCalendar *calendar, gpointer user_data);
static void callbk_calendar_prev_year(CustomCalendar *calendar, gpointer user_data);

static void update_label_date(CustomCalendar *calendar, gpointer user_data);

//marks
static void set_marks_on_calendar(CustomCalendar *calendar, GArray *arry); 
static void set_holidays_on_calendar(CustomCalendar *calendar);

//speak
static void speak_events();
static void speak_time(gint hour, gint min);
static void callbk_speak(GSimpleAction* action, GVariant *parameter,gpointer user_data);

static gpointer thread_playraw(gpointer user_data);

static char* get_cardinal_string(int number);
static char* get_day_number_ordinal_string(int day);
static char* get_day_of_week(int day, int month, int year);
static char* get_event_title_word(char* word);

static guint get_dropdown_position(const gchar* summary);

//Help Callbks
static void callbk_info(GSimpleAction *action, GVariant *parameter,  gpointer user_data);
static void callbk_about(GSimpleAction* action, GVariant *parameter, gpointer user_data);

//listbox display
static GtkWidget *create_widget (gpointer item, gpointer user_data);
static void add_separator (GtkListBoxRow *row, GtkListBoxRow *before, gpointer data);
static void callbk_row_activated (GtkListBox  *listbox,	 GtkListBoxRow *row, gpointer user_data);
static void display_event_array(GArray *evt_arry);

static void show_notifications(GArray *evt_arry);
GArray*  get_upcoming_array(int upcoming_days);

//config
static char * m_config_file = NULL;

//public holidays UK
gboolean is_notable_date(int day);
char* get_notable_date_str(int day);
GDate* calculate_easter(gint year);
//GDate* calculate_bst_start(gint year);
GDate* calculate_bst_start();
GDate* calculate_bst_end();

//int get_number_of_events();
int  get_total_number_of_events();
int  get_number_of_day_events();

//helpers
static char* remove_semicolons (const char *text);
static char* remove_punctuations(const char *text);
static char *remove_commas(const char *text);
static char *lower_case(const char *text);

static GListStore *m_store=NULL;   //m_store is a Gio GListStore store (not GktListStore which is being depreciated)
static int m_id_selection=-1;
static int m_row_index=-1; //listbox row index
static int m_store_size=0;

//calendar preferences
static int m_12hour_format=1; //am pm hour format
static int m_show_end_time=0; //show end_time
static int m_show_location=1; //show location
static int m_holidays=1; //holidays
//talk preferences
static int m_talk =1;
static int m_talk_at_startup =0;
static int m_talk_upcoming=0;
static int m_talk_event_number=1;
static int m_talk_time=1;
static int m_talk_event_words=1; 
static int m_talk_rate=7000;
static int m_reset_preferences=0;

static int m_today_year=0;
static int m_today_month=0;
static int m_today_day=0;

static const char* m_location ="";
static const char* m_description ="todo";
static int m_start_year=0;
static int m_start_month=0;
static int m_start_day=0;
static float m_start_time=0.0; //float
static int m_start_hour=0;
static int m_start_min=0;
static int m_end_year=0; //multiday plumbing (not yet implemented)
static int m_end_month=0;
static int m_end_day=0;
static float m_end_time=0.0; //float
static int m_end_hour=0;
static int m_end_min=0;
static int m_priority=0;
static int m_is_yearly=0;
static int m_is_allday=0;
static int m_notification=0;
static int m_has_reminder=0; //plumbing for future updates
static int m_reminder_min=0; //plumbing for future updates

static int m_window_width=500;
static int m_window_height=400;

static const char* m_todaycolour="red";
static const char* m_eventcolour="brown";
static const char* m_holidaycolour="blue";


const char * const events[] = { 
	"Activity",	
	"Anniversary",	
	"Appointment",
	"Birthday",	
	"Cafe",  
	"Car", 
	"Dentist", 
	"Doctor", 
	"Driver",
	"Family", 
	"Funeral",
	"Garden",	
	"Holiday",
	"Hospital",	
	"Meeting", 
	"Meetup", 
	"Party", 
	"Payment",
	"Reminder", 
	"Restaurant", 
	"Task", 		
	"Travel",  
	"Visit",
	"Work",  
	NULL };

const char* m_summary ="Activity";

CalendarEvent *selected_evt; 

const GActionEntry app_actions[] = {
  { "speak", callbk_speak },  
  { "speaktime", callbk_calendar_speaktime},
  { "home", callbk_calendar_home},
  { "newevent", callbk_new_event},
  { "editevent", callbk_edit_event},  
  { "deleteevent", callbk_delete_selected},
  { "info", callbk_info},
  { "preferences", callbk_preferences},
  { "quit", callbk_quit}
};

//======================================================================
//Helpers
//======================================================================
//---------------------------------------------------------------------
// GlistStore helper functions (Debugging)
//---------------------------------------------------------------------

gchar* get_gliststore_labels(GListStore *store) {


	gchar* events_str="";
	guint n_items;
    GListModel *model;
    gint id_value;
    gchar* label_value;
   

    model = G_LIST_MODEL (store);
    n_items = g_list_model_get_n_items (G_LIST_MODEL (model));

    for (int i=0; i<n_items; i++) {
        DisplayItem *test = g_list_model_get_item (model, i);
        g_object_get (test, "id", &id_value, NULL);
        g_object_get (test, "label", &label_value, NULL);
       // g_print("store at index %d: id = %d label =%s\n",i, id_value, label_value);

		events_str=g_strconcat(events_str, " ", label_value, " ",NULL);
    }

	return events_str;
}

int get_gliststore_number(GListStore *store) {

	guint n_items;
    GListModel *model;
    model = G_LIST_MODEL (store);
    n_items = g_list_model_get_n_items (G_LIST_MODEL (model));

	return n_items;

}

//======================================================================
// Save load config file
//======================================================================

static void config_load_default()
{
	//talking	
	m_talk=1;
	m_talk_at_startup=0;
	m_talk_upcoming=0;
	m_talk_event_number=1;
	m_talk_event_words=1;
	m_talk_time=1;
	m_talk_rate=7000;
		
	//calendar
	m_12hour_format=1;
	m_show_end_time=0;
	m_show_location=1;
	m_holidays=1;
	m_window_width=500;
	m_window_height=500;
	m_todaycolour="red";
    m_eventcolour="brown";
    m_holidaycolour="blue";	
}
//======================================================================
static void config_read()
{
	// Clean up previously loaded configuration values	

	m_talk=1;
	m_talk_at_startup=0;
	m_talk_upcoming=0;
	m_talk_event_number=1;
	m_talk_event_words=1;
	m_talk_time=1;
	m_talk_rate=7000;
	
	m_12hour_format=1;
	m_show_end_time=0;
	m_show_location=1;
	m_holidays=1;
	m_window_width=500;
	m_window_height=500;
	m_todaycolour="red";
    m_eventcolour="brown";
    m_holidaycolour="blue";
	// Load keys from keyfile
	GKeyFile * kf = g_key_file_new();
	g_key_file_load_from_file(kf, m_config_file, G_KEY_FILE_NONE, NULL);

	//talk		
	m_talk = g_key_file_get_integer(kf, "calendar_settings", "talk", NULL);
	m_talk_at_startup=g_key_file_get_integer(kf, "calendar_settings", "talk_startup", NULL);
	m_talk_upcoming=g_key_file_get_integer(kf, "calendar_settings", "talk_upcoming", NULL);
	m_talk_event_number=g_key_file_get_integer(kf, "calendar_settings", "talk_event_number", NULL);
	m_talk_event_words=g_key_file_get_integer(kf, "calendar_settings", "talk_event_words", NULL);
	m_talk_time=g_key_file_get_integer(kf, "calendar_settings", "talk_time", NULL);
	m_talk_rate=g_key_file_get_integer(kf, "calendar_settings", "talk_rate", NULL);
	
	m_12hour_format=g_key_file_get_integer(kf, "calendar_settings", "hour_format", NULL);
	m_show_end_time = g_key_file_get_integer(kf, "calendar_settings", "show_end_time", NULL);
	m_show_location = g_key_file_get_integer(kf, "calendar_settings", "show_location", NULL);

	m_holidays = g_key_file_get_integer(kf, "calendar_settings", "holidays", NULL);
	m_window_width = g_key_file_get_integer(kf, "calendar_settings", "window_width", NULL);
	m_window_height=g_key_file_get_integer(kf, "calendar_settings", "window_height", NULL);
	
	m_todaycolour=g_key_file_get_string(kf, "calendar_settings", "todaycolour", NULL);
    m_eventcolour=g_key_file_get_string(kf, "calendar_settings", "eventcolour", NULL);
    m_holidaycolour=g_key_file_get_string(kf, "calendar_settings", "holidaycolour", NULL);
	
	g_key_file_free(kf);
}
//======================================================================
void config_write()
{
	GKeyFile * kf = g_key_file_new();
	//talk		
	g_key_file_set_integer(kf, "calendar_settings", "talk", m_talk);
	g_key_file_set_integer(kf, "calendar_settings", "talk_startup", m_talk_at_startup);
	g_key_file_set_integer(kf, "calendar_settings", "talk_upcoming", m_talk_upcoming);
	g_key_file_set_integer(kf, "calendar_settings", "talk_event_number", m_talk_event_number);
	g_key_file_set_integer(kf, "calendar_settings", "talk_event_words", m_talk_event_words);
	g_key_file_set_integer(kf, "calendar_settings", "talk_time", m_talk_time);
	g_key_file_set_integer(kf, "calendar_settings", "talk_rate", m_talk_rate);
	
	g_key_file_set_integer(kf, "calendar_settings", "hour_format", m_12hour_format);
	g_key_file_set_integer(kf, "calendar_settings", "show_end_time", m_show_end_time);
	g_key_file_set_integer(kf, "calendar_settings", "show_location", m_show_location);

	g_key_file_set_integer(kf, "calendar_settings", "holidays", m_holidays);
	g_key_file_set_integer(kf, "calendar_settings", "window_width", m_window_width);
	g_key_file_set_integer(kf, "calendar_settings", "window_height", m_window_height);
	
	g_key_file_set_string(kf, "calendar_settings", "todaycolour", m_todaycolour);
	g_key_file_set_string(kf, "calendar_settings", "eventcolour", m_eventcolour);
	g_key_file_set_string(kf, "calendar_settings", "holidaycolour", m_holidaycolour);

	gsize length;
	gchar * data = g_key_file_to_data(kf, &length, NULL);
	g_file_set_contents(m_config_file, data, -1, NULL);
	g_free(data);
	g_key_file_free(kf);
}
//======================================================================
void config_initialize()
{
	gchar *config_dir = g_build_filename(g_get_user_config_dir(), CONFIG_DIRNAME, NULL);
	m_config_file = g_build_filename(config_dir, CONFIG_FILENAME, NULL);

	// Make sure config directory exists
	if (!g_file_test(config_dir, G_FILE_TEST_IS_DIR))
		// If a config file doesn't exist, create one with defaults
		g_mkdir(config_dir, 0777);
	// otherwise read the existing one
	if (!g_file_test(m_config_file, G_FILE_TEST_EXISTS))
	{
		config_load_default();
		config_write();
	}
	else
	{
		config_read();
	}

	g_free(config_dir);
}

//======================================================================
// Remove unwanted characters
//======================================================================

static char *remove_commas(const char *text)
{
	GString *str;
	const char *p;
	str = g_string_new("");
	p = text;
	while (*p)
	{
		gunichar cp = g_utf8_get_char(p);
		if (cp != ',')
		{ // do not allow commas into csv
			g_string_append_unichar(str, *p);
		} // if
		++p;
	}
	return g_string_free(str, FALSE);
}
//======================================================================
static char* remove_semicolons (const char *text)
{
	GString *str;
	const char *p;
	str = g_string_new ("");
	p = text;
	while (*p)
	{
	gunichar cp = g_utf8_get_char(p);
	if ( cp != ';' ){ //do not allow semicolons into database
	g_string_append_unichar (str, *p);
	}//if
	++p;
	}
	return g_string_free (str, FALSE);
}
//======================================================================
static char* remove_punctuations(const char *text)
{
	GString *str;
	const char *p;
	str = g_string_new ("");
	p = text;
	while (*p)
	{
	gunichar cp = g_utf8_get_char(p);
	if ( cp != '\'' ){ //remove all apostrophes as cause tts errors
	g_string_append_unichar (str, *p);
	}//if
	++p;
	}
	return g_string_free (str, FALSE);
}
//======================================================================
// Number of day events
//======================================================================

int  get_number_of_day_events(){
	int event_count=db_get_number_of_rows_year_month_day(m_start_year, m_start_month, m_start_day);	
	return event_count;
}

//======================================================================
int  get_total_number_of_events(){

	return db_get_number_of_rows_all();
}


//======================================================================
// Public holidays
//======================================================================

//======================================================================
// calculate BST
//======================================================================
GDate* calculate_bst_start()
{
	GDate *bst_date;	
	gint month =3;
	gint year=m_start_year;	
	
	GDate *last_sunday_march_date;
	guint8 days_in_march =g_date_get_days_in_month(month,year);
	
	//g_print("calculate_bst_start: month = %i\n",month);
	//g_print("calculate_bst_start: year = %i\n",year);
	//g_print("calculate_bst_start: days in march = %i\n",days_in_march);
    last_sunday_march_date = g_date_new_dmy(days_in_march, month, year);

     while (g_date_get_weekday(last_sunday_march_date) != G_DATE_SUNDAY)
       g_date_subtract_days(last_sunday_march_date,1);

    int last_sunday = g_date_get_day(last_sunday_march_date);
	//g_print("calculate_bst_start: last sunday = %i\n",last_sunday);
	bst_date = g_date_new_dmy(last_sunday, month, year);
	return bst_date;
}

GDate* calculate_bst_end()
{
	GDate *bst_date;	
	gint month =10;
	gint year=m_start_year;	
	
	GDate *last_sunday_october_date;
	guint8 days_in_october =g_date_get_days_in_month(month,year);
	
	//g_print("calculate_bst_end: month = %i\n",month);
	//g_print("calculate_bst_end: year = %i\n",year);
	//g_print("calculate_bst_end: days in october= %i\n",days_in_october);
    last_sunday_october_date = g_date_new_dmy(days_in_october, month, year);

     while (g_date_get_weekday(last_sunday_october_date) != G_DATE_SUNDAY)
       g_date_subtract_days(last_sunday_october_date,1);

    int last_sunday = g_date_get_day(last_sunday_october_date);
	//g_print("calculate_bst_end: last sunday = %i\n",last_sunday);
	bst_date = g_date_new_dmy(last_sunday, month, year);
	return bst_date;
	
}


//======================================================================
// calculate easter
//======================================================================

GDate* calculate_easter(gint year) {

	GDate *edate;

	gint Yr = year;
    gint a = Yr % 19;
    gint b = Yr / 100;
    gint c = Yr % 100;
    gint d = b / 4;
    gint e = b % 4;
    gint f = (b + 8) / 25;
    gint g = (b - f + 1) / 3;
    gint h = (19 * a + b - d - g + 15) % 30;
    gint i = c / 4;
    gint k = c % 4;
    gint L = (32 + 2 * e + 2 * i - h - k) % 7;
    gint m = (a + 11 * h + 22 * L) / 451;
    gint month = (h + L - 7 * m + 114) / 31;
    gint day = ((h + L - 7 * m + 114) % 31) + 1;
	edate = g_date_new_dmy(day, month, year);

	return edate;
}
//======================================================================
// public holidays
//======================================================================
gboolean is_notable_date(int day) {

// UK public holidays
// New Year's Day: 1 January (DONE)
// Good Friday: March or April  (DONE)
// Easter Monday: March or April (DONE)
// Early May: First Monday of May (DONE)
// Spring Bank Holiday: Last Monday of May (DONE)
// Summer Bank Holiday: Last Monday of August (DONE)
// Christmas Day: 25 December (DONE)
// Boxing day: 26 December (DONE)

	//markup public holidays
	if (m_start_month==1 && day ==1) {
	//new year
	 return TRUE;
	}

	if (m_start_month==12 && day==25) {
	//christmas day
	return TRUE;
	}

	if (m_start_month==12 && day==26) {
	//boxing day
	return TRUE;
	}
	
	if (m_start_month==2 && day==14) {
	//valentine day
	return TRUE;
	}
	
	GDate *bst_start_date =calculate_bst_start();
	GDate *bst_end_date =calculate_bst_end();
	
	int bst_start_day = g_date_get_day(bst_start_date);
	int bst_start_month =g_date_get_month(bst_start_date);
	
	if(m_start_month==bst_start_month && day == bst_start_day)
	{
	//bst starts
	return TRUE;
	}
	
	int bst_end_day = g_date_get_day(bst_end_date);
	int bst_end_month =g_date_get_month(bst_end_date);
	
	if(m_start_month==bst_end_month && day == bst_end_day)
	{
	//bst ends
	return TRUE;
	}
	
	//int bst_start_day = g_date_get_day(bst_start_date);
	//int bst_start_month =g_date_get_month(bst_start_date);
	//int bst_start_year =g_date_get_year(bst_start_date);
	//g_print("bst_start_date = %d-%d-%d\n",bst_start_day,bst_start_month,bst_start_year);
	
	//int bst_end_day = g_date_get_day(bst_end_date);
	//int bst_end_month =g_date_get_month(bst_end_date);
	//int bst_end_year =g_date_get_year(bst_end_date);
	//g_print("bst_end_date = %d-%d-%d\n",bst_end_day,bst_end_month,bst_end_year);
	
	

	if (m_start_month == 5) {
     //May complicated
     GDate *first_monday_may;
     first_monday_may = g_date_new_dmy(1, m_start_month, m_start_year);

     while (g_date_get_weekday(first_monday_may) != G_DATE_MONDAY)
       g_date_add_days(first_monday_may,1);

     int may_day = g_date_get_day(first_monday_may);

     if( day==may_day) return TRUE;
     //else return FALSE;

     int days_in_may =g_date_get_days_in_month (m_start_month, m_start_year);
     int plus_days = 0;

     if (may_day + 28 <= days_in_may) {
       plus_days = 28;
     } else {
       plus_days = 21;
     }

     GDate *spring_bank =g_date_new_dmy (may_day, m_start_month, m_start_year);

     g_date_add_days(spring_bank,plus_days);

     int spring_bank_day = g_date_get_day(spring_bank);

     if (g_date_valid_dmy (spring_bank_day,m_start_month,m_start_year) && day ==spring_bank_day)
     return TRUE;
	} //m_start_month==5 (may)

	GDate *easter_date =calculate_easter(m_start_year);
	int easter_day = g_date_get_day(easter_date);
	int easter_month =g_date_get_month(easter_date);

	if(m_start_month==easter_month && day == easter_day)
	{
	//easter day
	return TRUE;
	}
	g_date_subtract_days(easter_date,2);
	int easter_friday = g_date_get_day(easter_date);
	int easter_friday_month =g_date_get_month(easter_date);

	if(m_start_month==easter_friday_month && day ==easter_friday)
	{
	//easter friday
	return TRUE;
	}

	g_date_add_days(easter_date,3);
	int easter_monday = g_date_get_day(easter_date); //easter monday
	int easter_monday_month =g_date_get_month(easter_date);

	if(m_start_month==easter_monday_month && day ==easter_monday)
	{
	//easter monday
	return TRUE;
	}

	if (m_start_month == 8) {
      //August complicated
    GDate *first_monday_august;
     first_monday_august = g_date_new_dmy(1, m_start_month, m_start_year);

     while (g_date_get_weekday(first_monday_august) != G_DATE_MONDAY)
       g_date_add_days(first_monday_august,1);

     int august_day = g_date_get_day(first_monday_august);


     int days_in_august =g_date_get_days_in_month (m_start_month, m_start_year);
     int plus_days = 0;

     if (august_day + 28 <= days_in_august) {
       plus_days = 28;
     } else {
       plus_days = 21;
     }

     GDate *august_bank =g_date_new_dmy (august_day, m_start_month, m_start_year);

     g_date_add_days(august_bank,plus_days);

     int august_bank_day = g_date_get_day(august_bank);

     if (g_date_valid_dmy (august_bank_day,m_start_month,m_start_year) && day ==august_bank_day)
     return TRUE;
    } //m_start_month==8

	return FALSE;
}

char* get_notable_date_str(int day) {

// UK public holidays
// New Year's Day: 1 January (DONE)
// Good Friday: March or April  (DONE)
// Easter Monday: March or April (DONE)
// Early May: First Monday of May (TODO)
// Spring Bank Holiday: Last Monday of May (DONE)
// Summer Bank Holiday: Last Monday of August (DONE)
// Christmas Day: 25 December (DONE)
// Boxing day: 26 December (DONE)

char *date_str="";

	//markup public holidays
	if (m_start_month==1 && day ==1) {
	//return " New Year ";
	date_str =g_strconcat(date_str," New Year ", NULL); 
	//return " New Year ";
	}

	if (m_start_month==12 && day==25) {
	//christmas day
	//return " Christmas Day ";
	date_str =g_strconcat(date_str," Christmas Day ", NULL);
	}

	if (m_start_month==12 && day==26) {
	//boxing day
	//date_str=" Boxing Day ";
	date_str =g_strconcat(date_str," Boxing Day ", NULL);
	}

	if (m_start_month == 5) {
     //May complicated
     GDate *first_monday_may;
     first_monday_may = g_date_new_dmy(1, m_start_month, m_start_year);


     while (g_date_get_weekday(first_monday_may) != G_DATE_MONDAY)
       g_date_add_days(first_monday_may,1);

     int may_day = g_date_get_day(first_monday_may);

     if( day==may_day) 
     //return "Bank Holiday "; //may bank holiday
     date_str =g_strconcat(date_str," Bank Holiday ", NULL);

     int days_in_may =g_date_get_days_in_month (m_start_month, m_start_year);

     int plus_days = 0;

     if (may_day + 28 <= days_in_may) {
       plus_days = 28;
     } else {
       plus_days = 21;
     }

     GDate *spring_bank =g_date_new_dmy (may_day, m_start_month, m_start_year);
     g_date_add_days(spring_bank,plus_days);
     int spring_bank_day = g_date_get_day(spring_bank);
     if (g_date_valid_dmy (spring_bank_day,m_start_month,m_start_year) && day ==spring_bank_day)
     date_str= " Spring Bank Holiday ";   //spring bank holiday

	} //m_start_month ==5 (May)
	
	if (m_start_month==2 && day==14) {
	//valentine day
	//date_str= "Valentine Day ";
	date_str =g_strconcat(date_str," Valentine Day ", NULL);
	}
		
	GDate *easter_date =calculate_easter(m_start_year);
	int easter_day = g_date_get_day(easter_date);
	int easter_month =g_date_get_month(easter_date);

	if(m_start_month==easter_month && day == easter_day)
	{
	//easter day
	//return " Easter Day ";
	date_str =g_strconcat(date_str," Easter Day", NULL);
	}

	g_date_subtract_days(easter_date,2);
	int easter_friday = g_date_get_day(easter_date);
	int easter_friday_month =g_date_get_month(easter_date);

	if(m_start_month==easter_friday_month && day ==easter_friday)
	{
	//easter friday
	//return " Easter Friday ";
	date_str =g_strconcat(date_str,"Easter Friday ", NULL);
	}

	g_date_add_days(easter_date,3);
	int easter_monday = g_date_get_day(easter_date); //easter monday
	int easter_monday_month =g_date_get_month(easter_date);

	if(m_start_month==easter_monday_month && day ==easter_monday)
	{
	//easter monday
	//return " Easter Monday ";
	date_str =g_strconcat(date_str," Easter Monday", NULL);
	}

	GDate *bst_start_date =calculate_bst_start();
	int bst_start_day = g_date_get_day(bst_start_date);
	int bst_start_month =g_date_get_month(bst_start_date);
	
	if(m_start_month==bst_start_month && day == bst_start_day)
	{
	//bst starts
	//date_str= "BST Starts Clock Forward";
	date_str =g_strconcat(date_str," BST Begins Clocks Forward ", NULL);
	}
		
	
	if (m_start_month == 8) {
      //August complicated
    GDate *first_monday_august;
     first_monday_august = g_date_new_dmy(1, m_start_month, m_start_year);

     while (g_date_get_weekday(first_monday_august) != G_DATE_MONDAY)
       g_date_add_days(first_monday_august,1);

     int august_day = g_date_get_day(first_monday_august);


     int days_in_august =g_date_get_days_in_month (m_start_month, m_start_year);
     int plus_days = 0;

     if (august_day + 28 <= days_in_august) {
       plus_days = 28;
     } else {
       plus_days = 21;
     }

     GDate *august_bank =g_date_new_dmy (august_day, m_start_month, m_start_year);

     g_date_add_days(august_bank,plus_days);

     int august_bank_day = g_date_get_day(august_bank);

     if (g_date_valid_dmy (august_bank_day,m_start_month,m_start_year) && day ==august_bank_day)
     //return " Bank Holiday ";   //august bank holiday
     date_str =g_strconcat(date_str," Bank Holiday ", NULL);

    } //m_start_month==8
    
    
    GDate *bst_end_date =calculate_bst_end();
    int bst_end_day = g_date_get_day(bst_end_date);
	int bst_end_month =g_date_get_month(bst_end_date);
	
	if(m_start_month==bst_end_month && day == bst_end_day)
	{
	//bst ends
	//date_str= "BST Ends Clocks Back";
	date_str =g_strconcat(date_str," BST Ends Clocks Back", NULL);
	}

	//return "";
	return date_str;
}

//======================================================================
//Implementation of file menu callbks
//======================================================================

static void callbk_export(GSimpleAction *action, GVariant *parameter,  gpointer user_data)
{
	
	export_csv_file();
}
//======================================================================
static void callbk_import(GSimpleAction *action, GVariant *parameter,  gpointer user_data)
{
	
	import_csv_file(user_data);
}
//======================================================================
void export_csv_file() 
{	
	GFile *file;
	gchar *file_name = "events.csv";

	GFileOutputStream *file_stream;
	GDataOutputStream *data_stream;
	GError *err = NULL;
	
	file = g_file_new_for_path(file_name);

	file_stream = g_file_replace(file, 0,TRUE, G_FILE_CREATE_NONE, NULL, &err);
	    
	if (file_stream == NULL) {
                
				//gint errno = err->code;
				g_warning ("Error message = %s", err->message);
                g_error_free (err);
                g_object_unref (file);
                return;
        }

	data_stream = g_data_output_stream_new(G_OUTPUT_STREAM(file_stream));
	
	GArray *evt_arry =g_array_new(FALSE, FALSE, sizeof(CALENDAR_TYPE_EVENT));
	//db_reset_sequence();
	db_get_all_events(evt_arry);	

	for (int i = 0; i < evt_arry->len; i++)
	{

		gchar *line = "";
		gint evt_id = 0;
		gchar *summary_str = "";
		gchar *location_str = "";
		gchar *description_str = "";

		gint start_year = 0;
		gint start_month = 0;
		gint start_day = 0;
		gint start_hour = 0;
		gint start_min = 0;

		gint end_year = 0;
		gint end_month = 0;
		gint end_day = 0;
		gint end_hour = 0;
		gint end_min = 0;

		gint is_yearly = 0;
		gint is_allday = 0;
		gint is_priority = 0;
		gint has_notification = 0;
		gint has_reminder = 0;
		gint reminder_min = 0;

		CalendarEvent *evt = g_array_index(evt_arry, CalendarEvent *, i);

		g_object_get(evt, "eventid", &evt_id, NULL);
		g_object_get(evt, "summary", &summary_str, NULL);
		g_object_get(evt, "location", &location_str, NULL);
		g_object_get(evt, "description", &description_str, NULL);
		g_object_get(evt, "startyear", &start_year, NULL);
		g_object_get(evt, "startmonth", &start_month, NULL);
		g_object_get(evt, "startday", &start_day, NULL);
		g_object_get(evt, "starthour", &start_hour, NULL);
		g_object_get(evt, "startmin", &start_min, NULL);
		g_object_get(evt, "endyear", &end_year, NULL);
		g_object_get(evt, "endmonth", &end_month, NULL);
		g_object_get(evt, "endday", &end_day, NULL);
		g_object_get(evt, "endhour", &end_hour, NULL);
		g_object_get(evt, "endmin", &end_min, NULL);
		g_object_get(evt, "isyearly", &is_yearly, NULL);
		g_object_get(evt, "isallday", &is_allday, NULL);
		g_object_get(evt, "ispriority", &is_priority, NULL);
		g_object_get(evt, "hasnotification", &has_notification, NULL);
		g_object_get(evt, "hasreminder", &has_reminder, NULL);
		g_object_get(evt, "remindermin", &reminder_min, NULL);

		gchar *id_str = g_strdup_printf("%d", evt_id);
		gchar *start_year_str = g_strdup_printf("%d", start_year);
		gchar *start_month_str = g_strdup_printf("%d", start_month);
		gchar *start_day_str = g_strdup_printf("%d", start_day);

		gchar *start_hour_str = g_strdup_printf("%d", start_hour);
		gchar *start_min_str = g_strdup_printf("%d", start_min);

		gchar *end_year_str = g_strdup_printf("%d", end_year); // to do
		gchar *end_month_str = g_strdup_printf("%d", end_month);
		gchar *end_day_str = g_strdup_printf("%d", end_day);

		gchar *end_hour_str = g_strdup_printf("%d", end_hour);
		gchar *end_min_str = g_strdup_printf("%d", end_min);

		gchar *isyearly_str = g_strdup_printf("%d", is_yearly);
		gchar *isallday_str = g_strdup_printf("%d", is_allday);
		gchar *ispriority_str = g_strdup_printf("%d", is_priority);
		gchar *hasnotification_str = g_strdup_printf("%d", has_notification);
		gchar *hasreminder_str = g_strdup_printf("%d", has_reminder);
		gchar *remindermin_str = g_strdup_printf("%d", reminder_min);

		line = g_strconcat(line,
						   id_str, ",",
						   summary_str, ",",
						   location_str, ",",
						   description_str, ",",
						   start_year_str, ",",
						   start_month_str, ",",
						   start_day_str, ",",
						   start_hour_str, ",",
						   start_min_str, ",",
						   end_year_str, ",",
						   end_month_str, ",",
						   end_day_str, ",",
						   end_hour_str, ",",
						   end_min_str, ",",
						   isyearly_str, ",",
						   isallday_str, ",",
						   ispriority_str, ",",
						   hasnotification_str, ",",
						   hasreminder_str, ",",
						   remindermin_str, ",",
						   "\n",
						   NULL);

		g_data_output_stream_put_string(data_stream, line, NULL, NULL);
	}

	g_object_unref(data_stream);
	g_object_unref(file_stream);
	g_object_unref(file);
	
}
//======================================================================
int break_fields(char *s, char **data, int n)
{
	// n = number of fields
	// assumes comma delimiter

	int fields = 0;
	int i;
	char *start = s; // start at begiining of string
	char *end = s;	 // walking pointer

	for (i = 0; i < n; i++)
	{
		while (*end != ',' && *end != '\0')
		{
			end++;
		}

		if (*end == '\0')
		{

			data[i] = (char *)malloc(strlen(start) + 1);
			strcpy(data[i], start);
			fields++;
			break;
		}
		else if (*end == ',')
		{
			*end = '\0';
			data[i] = (char *)malloc(strlen(start) + 1);
			strcpy(data[i], start);
			start = end + 1;
			end = start;
			fields++;
		}
	}
	return fields;
}
//======================================================================
void import_csv_file(gpointer user_data) {
	
	GtkWidget *window = user_data;
	GtkWidget *calendar = g_object_get_data(G_OBJECT(window), "window-calendar-key");
		
	int count = 0;
	int field_num = 20;		 // fix for now?
	char *data[field_num];	 // fields
	int i = 0;				 // counter
	int total_num_lines = 0; // total number of lines
	int ret;

	GFile *file;
	GFileInputStream *file_stream = NULL;
	GDataInputStream *input = NULL;

	file = g_file_new_for_path("events.csv");

	file_stream = g_file_read(file, NULL, NULL);
	if (!file_stream)
	{
		g_print("CRITICAL: error: unable to open backup file called events.csv\n");
		return;
	}

	input = g_data_input_stream_new(G_INPUT_STREAM(file_stream));
	
	GArray *evt_arry =g_array_new(FALSE, FALSE, sizeof(CALENDAR_TYPE_EVENT));

	while (TRUE)
	{

		char *line;
		line = g_data_input_stream_read_line(input, NULL, NULL, NULL);
		if (line == NULL)
			break;

		// Event
		CalendarEvent *evt = g_object_new(CALENDAR_TYPE_EVENT, 0);

		ret = break_fields(line, data, field_num);

		for (int j = 0; j < field_num; j++)
		{

			if (j == 0)
				g_object_set(evt, "eventid", count, NULL);
			if (j == 1)
				g_object_set(evt, "summary", g_strdup(data[j]), NULL);
			if (j == 2)
				g_object_set(evt, "location", g_strdup(data[j]), NULL);
			if (j == 3)
				g_object_set(evt, "description", g_strdup(data[j]), NULL);
			if (j == 4)
				g_object_set(evt, "startyear", atoi(data[j]), NULL);
			if (j == 5)
				g_object_set(evt, "startmonth", atoi(data[j]), NULL);
			if (j == 6)
				g_object_set(evt, "startday", atoi(data[j]), NULL);
			if (j == 7)
				g_object_set(evt, "starthour", atoi(data[j]), NULL);
			if (j == 8)
				g_object_set(evt, "startmin", atoi(data[j]), NULL);
			if (j == 9)
				g_object_set(evt, "endyear", atoi(data[j]), NULL); // to do
			if (j == 10)
				g_object_set(evt, "endmonth", atoi(data[j]), NULL);
			if (j == 11)
				g_object_set(evt, "endday", atoi(data[j]), NULL);
			if (j == 12)
				g_object_set(evt, "endhour", atoi(data[j]), NULL);
			if (j == 13)
				g_object_set(evt, "endmin", atoi(data[j]), NULL);
			if (j == 14)
				g_object_set(evt, "isyearly", atoi(data[j]), NULL);
			if (j == 15)
				g_object_set(evt, "isallday", atoi(data[j]), NULL);
			if (j == 16)
				g_object_set(evt, "ispriority", atoi(data[j]), NULL);
			if (j == 17)
				g_object_set(evt, "hasnotification", atoi(data[j]), NULL);
			if (j == 18)
				g_object_set(evt, "hasreminder", atoi(data[j]), NULL);
			if (j == 19)
				g_object_set(evt, "remindermin", atoi(data[j]), NULL);

			free(data[j]);
		}

		g_array_append_val(evt_arry, evt);
		count = count + 1;
		i++;
	}
	
	total_num_lines = i;	
	//insert evt_array items into database
	for (int i = 0; i < evt_arry->len; i++)
	{
		CalendarEvent *evt = g_array_index(evt_arry, CalendarEvent *, i);
		db_insert_event(evt);
	}
	
	//update calendar	
	//g_list_store_remove_all(m_store); // clear	
	custom_calendar_reset_marks(CUSTOM_CALENDAR(calendar));		
	GArray *evt_arry_month; //add month marks
	evt_arry_month = g_array_new(FALSE, FALSE, sizeof(CALENDAR_TYPE_EVENT));
	db_get_all_events_year_month(evt_arry_month, m_start_year,m_start_month);	
	set_marks_on_calendar(CUSTOM_CALENDAR(calendar),evt_arry_month);
	g_array_free(evt_arry_month, FALSE); //clear the array
	custom_calendar_update(CUSTOM_CALENDAR(calendar));
	g_object_unref(file_stream);
	g_object_unref(file);	
}
//======================================================================

//======================================================================

static void callbk_quit(GSimpleAction * action,	G_GNUC_UNUSED GVariant *parameter, gpointer user_data)
{
	g_application_quit(G_APPLICATION(user_data));		
}
//======================================================================
//======================================================================
//Implementation of Edit menu callbks
//======================================================================
static void callbk_set_preferences(GtkButton *button, gpointer  user_data){

	GtkWidget *window = user_data;
	GtkWidget *calendar =g_object_get_data(G_OBJECT(window), "window-calendar-key");
	GtkWidget *label_date =g_object_get_data(G_OBJECT(window), "window-label-date-key");
	GtkWidget *dialog = g_object_get_data(G_OBJECT(button), "dialog-key");

	//calendar
	GtkWidget *check_button_hour_format= g_object_get_data(G_OBJECT(button), "check-button-hour-format-key");
	GtkWidget *check_button_show_end_time= g_object_get_data(G_OBJECT(button), "check-button-show-end-time-key");
	GtkWidget *check_button_show_location= g_object_get_data(G_OBJECT(button), "check-button-show-location-key");
	GtkWidget *check_button_holidays= g_object_get_data(G_OBJECT(button), "check-button-holidays-key");
	
	
	//talking
	GtkWidget *check_button_talk= g_object_get_data(G_OBJECT(button), "check-button-talk-key");
    GtkWidget *check_button_talk_startup= g_object_get_data(G_OBJECT(button), "check-button-talk-startup-key");
    GtkWidget *check_button_talk_upcoming= g_object_get_data(G_OBJECT(button), "check-button-talk-upcoming-key");
    GtkWidget *check_button_talk_event_number= g_object_get_data(G_OBJECT(button), "check-button-talk-event-number-key");
	GtkWidget *check_button_talk_time= g_object_get_data(G_OBJECT(button), "check-button-talk-time-key");
	GtkWidget *check_button_talk_event_words=g_object_get_data(G_OBJECT(button), "check-button-talk-event-words-key");	
	GtkWidget *spin_button_talk_rate = g_object_get_data(G_OBJECT(button), "spin-talk-rate-key");
	
    GtkWidget *check_button_reset_all= g_object_get_data(G_OBJECT(button), "check-button-reset-all-key");

	m_12hour_format=gtk_check_button_get_active (GTK_CHECK_BUTTON(check_button_hour_format));
	m_show_end_time=gtk_check_button_get_active (GTK_CHECK_BUTTON(check_button_show_end_time));
	m_show_location=gtk_check_button_get_active (GTK_CHECK_BUTTON(check_button_show_location));
	m_holidays=gtk_check_button_get_active (GTK_CHECK_BUTTON(check_button_holidays));
	
	m_talk=gtk_check_button_get_active (GTK_CHECK_BUTTON(check_button_talk));
	m_talk_at_startup=gtk_check_button_get_active (GTK_CHECK_BUTTON(check_button_talk_startup));
	m_talk_upcoming =gtk_check_button_get_active (GTK_CHECK_BUTTON(check_button_talk_upcoming));
	m_talk_event_number=gtk_check_button_get_active (GTK_CHECK_BUTTON(check_button_talk_event_number));
	m_talk_time =gtk_check_button_get_active (GTK_CHECK_BUTTON(check_button_talk_time));
	m_talk_event_words =gtk_check_button_get_active (GTK_CHECK_BUTTON(check_button_talk_event_words));
	m_talk_rate = gtk_spin_button_get_value(GTK_SPIN_BUTTON(spin_button_talk_rate));
	
		
	m_reset_preferences=gtk_check_button_get_active(GTK_CHECK_BUTTON(check_button_reset_all));

	if(m_reset_preferences) {
	//reset everything
	m_12hour_format=1;
	m_show_end_time=0;
	m_show_location=1;
	m_holidays=1;
	
	m_talk=1;	
	m_talk_rate=7000;
	m_talk_event_words=1;
	m_talk_time=1;	
	m_talk_at_startup=0;
	m_talk_upcoming=0;
	m_talk_event_number=1;
				
    m_reset_preferences=0; //toggle
	}
	
	config_write();	//save preferences
	
	//update everything
	
	custom_calendar_reset_marks(CUSTOM_CALENDAR(calendar));	
	custom_calendar_reset_holidays(CUSTOM_CALENDAR(calendar));
	if(m_holidays) set_holidays_on_calendar(CUSTOM_CALENDAR(calendar));
	custom_calendar_update(CUSTOM_CALENDAR(calendar));
	
	//update month events
	GArray *evt_arry_month; 
	evt_arry_month = g_array_new(FALSE, FALSE, sizeof(CALENDAR_TYPE_EVENT));
	db_get_all_events_year_month(evt_arry_month, m_start_year,m_start_month);	
	set_marks_on_calendar(CUSTOM_CALENDAR(calendar),evt_arry_month);
	g_array_free(evt_arry_month, FALSE); //clear the array
	
	//update day events	
	g_list_store_remove_all(m_store); // clear
	
	GArray *evt_arry_day;	
	evt_arry_day = g_array_new(FALSE, FALSE, sizeof(CALENDAR_TYPE_EVENT)); // setup arraylist
	db_get_all_events_year_month_day(evt_arry_day, m_start_year,m_start_month, m_start_day);	
	display_event_array(evt_arry_day);
	g_array_free(evt_arry_day, FALSE); //clear the array 
	
	custom_calendar_update(CUSTOM_CALENDAR(calendar));
	update_label_date(CUSTOM_CALENDAR(calendar), label_date);	
	
	gtk_window_destroy(GTK_WINDOW(dialog));
}
//======================================================================
static void callbk_preferences(GSimpleAction* action, GVariant *parameter,gpointer user_data)
{	
	GtkWidget *window =user_data;

	GtkWidget *dialog;
	GtkWidget *box;
	
	//Check buttons
	//calendar	
	GtkWidget *check_button_hour_format;
	GtkWidget *check_button_show_end_time;
	GtkWidget *check_button_show_location;
	GtkWidget *check_button_holidays;
	//talk
	GtkWidget *check_button_talk;
	GtkWidget *check_button_talk_startup;
	GtkWidget *check_button_talk_upcoming;
	GtkWidget *check_button_talk_event_number;
	GtkWidget *check_button_talk_time;
	GtkWidget *check_button_talk_event_words;	
	
	GtkWidget *label_talk_rate;
	GtkWidget *spin_button_talk_rate;
	GtkWidget *box_talk_rate;
		
	GtkWidget *check_button_reset_all;		
	GtkWidget *button_set;	

	dialog =gtk_window_new(); //gtk_dialog_new_with_buttons to be deprecated gtk4.10

	gtk_window_set_title (GTK_WINDOW (dialog), "Preferences");
	gtk_window_set_default_size(GTK_WINDOW(dialog),350,100);

	box =gtk_box_new(GTK_ORIENTATION_VERTICAL,1);
	gtk_window_set_child (GTK_WINDOW (dialog), box);

	button_set = gtk_button_new_with_label ("Set Preferences");
	g_signal_connect (button_set, "clicked", G_CALLBACK (callbk_set_preferences), window);

	//calendar
	check_button_hour_format = gtk_check_button_new_with_label ("12 Hour Format");
	check_button_show_end_time= gtk_check_button_new_with_label ("Show End Time");
	check_button_show_location= gtk_check_button_new_with_label ("Show Location");
	check_button_holidays = gtk_check_button_new_with_label ("Notable Dates");
	//talk
	check_button_talk = gtk_check_button_new_with_label ("Talk");
	check_button_talk_startup = gtk_check_button_new_with_label ("Talk At Startup");
	check_button_talk_upcoming= gtk_check_button_new_with_label ("Talk Upcoming");
	check_button_talk_event_number = gtk_check_button_new_with_label ("Talk Event Number");
	check_button_talk_time= gtk_check_button_new_with_label ("Talk Time");
	check_button_talk_event_words= gtk_check_button_new_with_label ("Talk Event Words");
		
	check_button_reset_all = gtk_check_button_new_with_label ("Reset All");

	//sample rate
	GtkAdjustment *adjustment_talk_rate;
	// value,lower,upper,step_increment,page_increment,page_size
	adjustment_talk_rate = gtk_adjustment_new(7000.00, 5000.00, 20000.00, 1000.0, 1000.0, 0.0);
	// start time spin
	label_talk_rate = gtk_label_new("Talk Rate ");
	spin_button_talk_rate = gtk_spin_button_new(adjustment_talk_rate, 1000, 0);
	gtk_spin_button_set_value(GTK_SPIN_BUTTON(spin_button_talk_rate), m_talk_rate);
	box_talk_rate = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 1);
	gtk_box_append(GTK_BOX(box_talk_rate), label_talk_rate);
	gtk_box_append(GTK_BOX(box_talk_rate), spin_button_talk_rate);

	//set calendar preferences
	gtk_check_button_set_active (GTK_CHECK_BUTTON(check_button_hour_format),m_12hour_format);
	gtk_check_button_set_active (GTK_CHECK_BUTTON(check_button_show_end_time), m_show_end_time);
	gtk_check_button_set_active (GTK_CHECK_BUTTON(check_button_show_location), m_show_location);
	gtk_check_button_set_active (GTK_CHECK_BUTTON(check_button_holidays),m_holidays);
	//set talk
	gtk_check_button_set_active (GTK_CHECK_BUTTON(check_button_talk), m_talk);
	gtk_check_button_set_active (GTK_CHECK_BUTTON(check_button_talk_startup), m_talk_at_startup);
	gtk_check_button_set_active (GTK_CHECK_BUTTON(check_button_talk_upcoming), m_talk_upcoming);
	gtk_check_button_set_active (GTK_CHECK_BUTTON(check_button_talk_event_number), m_talk_event_number);
	gtk_check_button_set_active (GTK_CHECK_BUTTON(check_button_talk_time), m_talk_time);
	gtk_check_button_set_active (GTK_CHECK_BUTTON(check_button_talk_event_words), m_talk_event_words);	
	
	gtk_check_button_set_active (GTK_CHECK_BUTTON(check_button_reset_all), m_reset_preferences);

	//data setters
	g_object_set_data(G_OBJECT(button_set), "dialog-key",dialog);
	//calendar
	g_object_set_data(G_OBJECT(button_set), "check-button-hour-format-key",check_button_hour_format);
	g_object_set_data(G_OBJECT(button_set), "check-button-show-end-time-key",check_button_show_end_time);
	g_object_set_data(G_OBJECT(button_set), "check-button-show-location-key",check_button_show_location);
	g_object_set_data(G_OBJECT(button_set), "check-button-holidays-key",check_button_holidays);
	//talk
	g_object_set_data(G_OBJECT(button_set), "check-button-talk-key",check_button_talk);
	g_object_set_data(G_OBJECT(button_set), "check-button-talk-startup-key",check_button_talk_startup);
	g_object_set_data(G_OBJECT(button_set), "check-button-talk-upcoming-key",check_button_talk_upcoming);
	g_object_set_data(G_OBJECT(button_set), "check-button-talk-event-number-key",check_button_talk_event_number);
	g_object_set_data(G_OBJECT(button_set), "check-button-talk-time-key",check_button_talk_time);
	g_object_set_data(G_OBJECT(button_set), "check-button-talk-event-words-key",check_button_talk_event_words);
	g_object_set_data(G_OBJECT(button_set), "spin-talk-rate-key", spin_button_talk_rate);

	g_object_set_data(G_OBJECT(button_set), "check-button-reset-all-key",check_button_reset_all);
	//calendar
	gtk_box_append(GTK_BOX(box), check_button_hour_format);
	gtk_box_append(GTK_BOX(box), check_button_show_end_time);
	gtk_box_append(GTK_BOX(box), check_button_show_location);
	gtk_box_append(GTK_BOX(box), check_button_holidays);
	//talk
	gtk_box_append(GTK_BOX(box), check_button_talk);		
	gtk_box_append(GTK_BOX(box), check_button_talk_event_words);	
	gtk_box_append(GTK_BOX(box), check_button_talk_time);
	gtk_box_append(GTK_BOX(box), check_button_talk_startup);
	gtk_box_append(GTK_BOX(box), check_button_talk_upcoming);
	gtk_box_append(GTK_BOX(box), check_button_talk_event_number);
	gtk_box_append(GTK_BOX(box), box_talk_rate);

	gtk_box_append(GTK_BOX(box), check_button_reset_all);
	gtk_box_append(GTK_BOX(box), button_set);
	gtk_window_present (GTK_WINDOW (dialog));
	
}
//======================================================================
//Implementation of event menu callbks
//======================================================================

static void callbk_check_button_allday_toggled(GtkCheckButton *check_button, gpointer user_data)
{

	GtkWidget *spin_button_start_time;
	GtkWidget *spin_button_end_time;

	spin_button_start_time = g_object_get_data(G_OBJECT(user_data), "cb_allday_spin_start_time_key");
	spin_button_end_time = g_object_get_data(G_OBJECT(user_data), "cb_allday_spin_end_time_key");

	if (gtk_check_button_get_active(GTK_CHECK_BUTTON(check_button)))
	{
		gtk_widget_set_sensitive(spin_button_start_time, FALSE);
		gtk_widget_set_sensitive(spin_button_end_time, FALSE);
	}
	else
	{
		gtk_widget_set_sensitive(spin_button_start_time, TRUE);
		gtk_widget_set_sensitive(spin_button_end_time, TRUE);
	}
}
//======================================================================
static void callbk_dropdown(GtkDropDown* self, gpointer user_data)
{	
	m_summary = gtk_string_object_get_string (GTK_STRING_OBJECT (gtk_drop_down_get_selected_item (self)));	
}
//======================================================================
static void callbk_add_event(GtkButton *button, gpointer user_data)
{

	GtkWidget *window = user_data;
	GtkWidget *calendar = g_object_get_data(G_OBJECT(window), "window-calendar-key");
	GtkWidget *label_date = g_object_get_data(G_OBJECT(window), "window-label-date-key");

	GtkWidget *dialog = g_object_get_data(G_OBJECT(button), "dialog-key");

	GtkEntryBuffer *buffer_summary;
	GtkWidget *entry_summary = g_object_get_data(G_OBJECT(button), "entry-summary-key");

	GtkEntryBuffer *buffer_location;
	GtkWidget *entry_location = g_object_get_data(G_OBJECT(button), "entry-location-key");

	GtkEntryBuffer *buffer_description;
	GtkWidget *entry_description = g_object_get_data(G_OBJECT(button), "entry-description-key");

	GtkWidget *spin_button_start_time = g_object_get_data(G_OBJECT(button), "spin-start-time-key");
	GtkWidget *spin_button_end_time = g_object_get_data(G_OBJECT(button), "spin-end-time-key");

	GtkWidget *check_button_allday = g_object_get_data(G_OBJECT(button), "check-button-allday-key");
	GtkWidget *check_button_isyearly = g_object_get_data(G_OBJECT(button), "check-button-isyearly-key");
	GtkWidget *check_button_priority = g_object_get_data(G_OBJECT(button), "check-button-priority-key");
	GtkWidget *check_button_notification = g_object_get_data(G_OBJECT(button), "check-button-notification-key");
		
	buffer_description = gtk_entry_get_buffer(GTK_ENTRY(entry_description));
	m_description = gtk_entry_buffer_get_text(buffer_description);
	m_description = remove_semicolons(m_description);
	m_description = remove_commas(m_description);
	m_description =remove_punctuations(m_description);
		
	buffer_location = gtk_entry_get_buffer(GTK_ENTRY(entry_location));
	m_location = gtk_entry_buffer_get_text(buffer_location);
	m_location = remove_semicolons(m_location);
	m_location = remove_commas(m_location);
	m_location =remove_punctuations(m_location);
		float start_time = gtk_spin_button_get_value(GTK_SPIN_BUTTON(spin_button_start_time));
	float end_time = gtk_spin_button_get_value(GTK_SPIN_BUTTON(spin_button_end_time));
	
	if(end_time <start_time) end_time=start_time;

	// convert to start hour and min
	float integral_part, fractional_part;
	fractional_part = modff(start_time, &integral_part);
	int start_hour = (int)integral_part; // start_hour
	fractional_part = round(fractional_part * 100);
	int start_min = (int)(fractional_part); // start_min
	// convert to end hour and min
	float integral_part_end, fractional_part_end;
	fractional_part_end = modff(end_time, &integral_part_end);
	int end_hour = (int)integral_part_end; // end_hour
	fractional_part_end = round(fractional_part_end * 100);
	int end_min = (int)(fractional_part_end); // start_min

	int isyearly = gtk_check_button_get_active(GTK_CHECK_BUTTON(check_button_isyearly));
	int isallday = gtk_check_button_get_active(GTK_CHECK_BUTTON(check_button_allday));
	int priority = gtk_check_button_get_active(GTK_CHECK_BUTTON(check_button_priority));
	int notification = gtk_check_button_get_active(GTK_CHECK_BUTTON(check_button_notification));
	
	CalendarEvent *evt = g_object_new(CALENDAR_TYPE_EVENT, 0);
	
	//g_object_set(evt, "eventid", id_idx, NULL);
	g_object_set(evt, "summary", g_strdup(m_summary), NULL);
	g_object_set(evt, "location", g_strdup(m_location), NULL);
	g_object_set(evt, "description", g_strdup(m_description), NULL);
	g_object_set(evt, "startyear", m_start_year, NULL);
	g_object_set(evt, "startmonth", m_start_month, NULL);
	g_object_set(evt, "startday", m_start_day, NULL);
	g_object_set(evt, "starthour", start_hour, NULL);
	g_object_set(evt, "startmin", start_min, NULL);
	g_object_set(evt, "endyear", m_end_year, NULL); // to do
	g_object_set(evt, "endmonth", m_end_month, NULL);
	g_object_set(evt, "endday", m_end_day, NULL);
	g_object_set(evt, "endhour", end_hour, NULL);
	g_object_set(evt, "endmin", end_min, NULL);
	g_object_set(evt, "isyearly", isyearly, NULL);
	g_object_set(evt, "isallday", isallday, NULL);	
	g_object_set(evt, "ispriority", priority, NULL);
	g_object_set(evt, "hasnotification", notification, NULL);
	g_object_set(evt, "hasreminder", 0, NULL);
	g_object_set(evt, "remindermin", 30, NULL);
	
	//Testing
	gint tevt_id=0;
	gchar *tsummary_str="";
	gchar *tlocation_str="";
	
	g_object_get (evt, "eventid", &tevt_id, NULL);
	g_object_get (evt, "summary", &tsummary_str, NULL);
	g_object_get (evt, "location", &tlocation_str, NULL);
		
	db_insert_event(evt); //insert event into database
	
	m_id_selection = -1;
	update_label_date(CUSTOM_CALENDAR(calendar), label_date);
	
	custom_calendar_reset_marks(CUSTOM_CALENDAR(calendar));		
	GArray *evt_arry_month; //add month marks
	evt_arry_month = g_array_new(FALSE, FALSE, sizeof(CALENDAR_TYPE_EVENT));
	db_get_all_events_year_month(evt_arry_month, m_start_year,m_start_month);	
	set_marks_on_calendar(CUSTOM_CALENDAR(calendar),evt_arry_month);
	g_array_free(evt_arry_month, FALSE); //clear the array
		
	custom_calendar_update(CUSTOM_CALENDAR(calendar));
	
	GArray *evt_arry_day;
	
	evt_arry_day = g_array_new(FALSE, FALSE, sizeof(CALENDAR_TYPE_EVENT)); // setup arraylist
	db_get_all_events_year_month_day(evt_arry_day, m_start_year,m_start_month, m_start_day);		
	display_event_array(evt_arry_day);
	g_array_free(evt_arry_day, FALSE); //clear the array 
	
	gtk_window_destroy(GTK_WINDOW(dialog));
}
//======================================================================
static void callbk_new_event(GSimpleAction *action, GVariant *parameter,  gpointer user_data)
{	
	GtkWidget *window = user_data;

	GtkWidget *dialog;
	gint response;
	GtkWidget *grid;
	GtkWidget *box;
	GtkWidget *label_date;
	GtkEntryBuffer *buffer;

	GtkWidget *label_entry_summary;	
	GtkWidget *dropdown;
		
	GtkWidget *label_location;
	GtkWidget *entry_location;

	// Description
	GtkWidget *label_description;
	GtkWidget *entry_description;

	// Start time
	GtkWidget *label_start_time;
	GtkWidget *spin_button_start_time;
	GtkWidget *box_start_time;

	// End time
	GtkWidget *label_end_time;
	GtkWidget *spin_button_end_time;
	GtkWidget *box_end_time;

	// Check buttons
	GtkWidget *check_button_allday;
	GtkWidget *check_button_isyearly;
	GtkWidget *check_button_priority;
	GtkWidget *check_button_notification;

	GtkWidget *button_add;

	GDate *event_date;
	event_date = g_date_new();
	event_date = g_date_new_dmy(m_start_day, m_start_month, m_start_year);
	int event_day = g_date_get_day(event_date);
	int event_month = g_date_get_month(event_date);
	int event_year = g_date_get_year(event_date);

	gchar *date_str = "Event Date: ";
	gchar *day_str = g_strdup_printf("%d", event_day);
	gchar *month_str = g_strdup_printf("%d", event_month);
	gchar *year_str = g_strdup_printf("%d", event_year);

	date_str = g_strconcat(date_str, day_str, "-", month_str, "-", year_str, NULL);

	dialog = gtk_window_new(); // gtk_dialog_new_with_buttons to be deprecated gtk4.10

	gtk_window_set_title(GTK_WINDOW(dialog), "New Event");
	gtk_window_set_default_size(GTK_WINDOW(dialog), 350, 100);

	box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 1);
	gtk_window_set_child(GTK_WINDOW(dialog), box);

	button_add = gtk_button_new_with_label("Add");
	g_signal_connect(button_add, "clicked", G_CALLBACK(callbk_add_event), window);

	label_date = gtk_label_new(date_str);

	label_entry_summary = gtk_label_new("Event Speech Word");
	
	dropdown =gtk_drop_down_new_from_strings(events);    
    g_signal_connect(GTK_DROP_DOWN(dropdown), "notify::selected", G_CALLBACK(callbk_dropdown), NULL);

	label_description = gtk_label_new("Event Details");
	entry_description = gtk_entry_new();
	gtk_entry_set_max_length(GTK_ENTRY(entry_description), 200);

	label_location = gtk_label_new("Location");
	entry_location = gtk_entry_new();
	gtk_entry_set_max_length(GTK_ENTRY(entry_location), 100);

	gtk_box_append(GTK_BOX(box), label_date);
	gtk_box_append(GTK_BOX(box), label_entry_summary);
	gtk_box_append(GTK_BOX(box), dropdown);
	
	gtk_box_append(GTK_BOX(box), label_description);
	gtk_box_append(GTK_BOX(box), entry_description);

	gtk_box_append(GTK_BOX(box), label_location);
	gtk_box_append(GTK_BOX(box), entry_location);

	g_object_set_data(G_OBJECT(button_add), "dialog-key", dialog);
	g_object_set_data(G_OBJECT(button_add), "entry-location-key", entry_location);
	g_object_set_data(G_OBJECT(button_add), "entry-description-key", entry_description);

	//--------------------------------------------------------
	// Start time spin buttons
	//---------------------------------------------------------

	GtkAdjustment *adjustment_start;
	// value,lower,upper,step_increment,page_increment,page_size
	adjustment_start = gtk_adjustment_new(8.00, 0.0, 23.59, 1.0, 1.0, 0.0);
	// start time spin
	label_start_time = gtk_label_new("Start Time (24 hour) ");
	spin_button_start_time = gtk_spin_button_new(adjustment_start, 1.0, 2);
	box_start_time = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 1);
	gtk_box_append(GTK_BOX(box_start_time), label_start_time);
	gtk_box_append(GTK_BOX(box_start_time), spin_button_start_time);
	gtk_box_append(GTK_BOX(box), box_start_time);

	g_object_set_data(G_OBJECT(button_add), "spin-start-time-key", spin_button_start_time);

	// end time spin
	GtkAdjustment *adjustment_end;
	adjustment_end = gtk_adjustment_new(8.00, 0.0, 23.59, 1.0, 1.0, 0.0);
	label_end_time = gtk_label_new("End Time (24 hour) ");
	spin_button_end_time = gtk_spin_button_new(adjustment_end, 1.0, 2);
	box_end_time = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 1);
	gtk_box_append(GTK_BOX(box_end_time), label_end_time);
	gtk_box_append(GTK_BOX(box_end_time), spin_button_end_time);
	gtk_box_append(GTK_BOX(box), box_end_time);

	g_object_set_data(G_OBJECT(button_add), "spin-end-time-key", spin_button_end_time);

	// check buttons
	check_button_allday = gtk_check_button_new_with_label("Is All Day");

	g_object_set_data(G_OBJECT(check_button_allday), "cb_allday_spin_start_time_key", spin_button_start_time);
	g_object_set_data(G_OBJECT(check_button_allday), "cb_allday_spin_end_time_key", spin_button_end_time);

	g_signal_connect_swapped(GTK_CHECK_BUTTON(check_button_allday), "toggled",
							 G_CALLBACK(callbk_check_button_allday_toggled), check_button_allday);

	check_button_isyearly = gtk_check_button_new_with_label("Is Yearly");
	check_button_priority = gtk_check_button_new_with_label("Is High Priority");
	check_button_notification = gtk_check_button_new_with_label("Send Notification");
	
	gtk_box_append(GTK_BOX(box), check_button_allday);
	gtk_box_append(GTK_BOX(box), check_button_isyearly);
	gtk_box_append(GTK_BOX(box), check_button_priority);
	gtk_box_append(GTK_BOX(box), check_button_notification);

	gtk_box_append(GTK_BOX(box), button_add);

	g_object_set_data(G_OBJECT(button_add), "check-button-allday-key", check_button_allday);
	g_object_set_data(G_OBJECT(button_add), "check-button-isyearly-key", check_button_isyearly);
	g_object_set_data(G_OBJECT(button_add), "check-button-priority-key", check_button_priority);
	g_object_set_data(G_OBJECT(button_add), "check-button-notification-key", check_button_notification);

	gtk_window_present(GTK_WINDOW(dialog));
	
}
//======================================================================

static void callbk_update_event(GtkButton *button, gpointer user_data)
{

	GtkWidget *window = user_data;
	GtkWidget *calendar = g_object_get_data(G_OBJECT(window), "window-calendar-key");
	GtkWidget *label_date = g_object_get_data(G_OBJECT(window), "window-label-date-key");

	GtkWidget *dialog = g_object_get_data(G_OBJECT(button), "dialog-key");

	GtkEntryBuffer *buffer_location;
	GtkWidget *entry_location = g_object_get_data(G_OBJECT(button), "entry-location-key");

	GtkEntryBuffer *buffer_description;
	GtkWidget *entry_description = g_object_get_data(G_OBJECT(button), "entry-description-key");

	GtkWidget *check_button_allday = g_object_get_data(G_OBJECT(button), "check-button-allday-key");
	GtkWidget *check_button_isyearly = g_object_get_data(G_OBJECT(button), "check-button-isyearly-key");
	GtkWidget *check_button_priority = g_object_get_data(G_OBJECT(button), "check-button-priority-key");
	GtkWidget *check_button_notification = g_object_get_data(G_OBJECT(button), "check-button-notification-key");

	
	GtkWidget *spin_button_start_time = g_object_get_data(G_OBJECT(button), "spin-start-time-key");
	GtkWidget *spin_button_end_time = g_object_get_data(G_OBJECT(button), "spin-end-time-key");
		
	buffer_location = gtk_entry_get_buffer(GTK_ENTRY(entry_location));
	m_location = gtk_entry_buffer_get_text(buffer_location);
	m_location = remove_semicolons(m_location);
	m_location = remove_commas(m_location);
	m_location =remove_punctuations(m_location);
	

	buffer_description = gtk_entry_get_buffer(GTK_ENTRY(entry_description));
	m_description = gtk_entry_buffer_get_text(buffer_description);
	m_description = remove_semicolons(m_description);
	m_description = remove_commas(m_description);
	m_description = remove_punctuations(m_description);

	// m_description="textview todo";
	float start_time = gtk_spin_button_get_value(GTK_SPIN_BUTTON(spin_button_start_time));
	float end_time = gtk_spin_button_get_value(GTK_SPIN_BUTTON(spin_button_end_time));
	
	if(end_time <start_time) end_time=start_time;

	// convert to start hour and min
	float integral_part, fractional_part;
	fractional_part = modff(start_time, &integral_part);
	m_start_hour = (int)integral_part; // start_hour
	fractional_part = round(fractional_part * 100);
	m_start_min = (int)(fractional_part); // start_min
	// convert to end hour and min
	float integral_part_end, fractional_part_end;
	fractional_part_end = modff(end_time, &integral_part_end);
	m_end_hour = (int)integral_part_end; // end_hour
	fractional_part_end = round(fractional_part_end * 100);
	m_end_min = (int)(fractional_part_end); // start_min

	m_is_yearly = gtk_check_button_get_active(GTK_CHECK_BUTTON(check_button_isyearly));
	m_is_allday = gtk_check_button_get_active(GTK_CHECK_BUTTON(check_button_allday));
	m_priority = gtk_check_button_get_active(GTK_CHECK_BUTTON(check_button_priority));
	m_notification = gtk_check_button_get_active(GTK_CHECK_BUTTON(check_button_notification));

	gint arry_index = 0;
	
	//g_object_set(selected_evt, "eventid", arry_index, NULL);
	g_object_set(selected_evt, "summary", g_strdup(m_summary), NULL);
	g_object_set(selected_evt, "location", g_strdup(m_location), NULL);
	g_object_set(selected_evt, "description", g_strdup(m_description), NULL);
	g_object_set(selected_evt, "startyear", m_start_year, NULL);
	g_object_set(selected_evt, "startmonth", m_start_month, NULL);
	g_object_set(selected_evt, "startday", m_start_day, NULL);
	g_object_set(selected_evt, "starthour", m_start_hour, NULL);
	g_object_set(selected_evt, "startmin", m_start_min, NULL);
	g_object_set(selected_evt, "endyear", m_end_year, NULL); // to do
	g_object_set(selected_evt, "endmonth", m_end_month, NULL);
	g_object_set(selected_evt, "endday", m_end_day, NULL);
	g_object_set(selected_evt, "endhour", m_end_hour, NULL);
	g_object_set(selected_evt, "endmin", m_end_min, NULL);
	g_object_set(selected_evt, "isyearly", m_is_yearly, NULL);
	g_object_set(selected_evt, "isallday", m_is_allday, NULL);
	g_object_set(selected_evt, "ispriority", m_priority, NULL);
	g_object_set(selected_evt, "hasnotification", m_notification, NULL);
	g_object_set(selected_evt, "hasreminder", m_has_reminder, NULL);
	g_object_set(selected_evt, "remindermin", m_reminder_min, NULL);
	
	//Testing
	//gint tevt_id=0;
	gchar *tsummary_str="";
	gchar *tlocation_str="";
		//g_object_get (evt, "eventid", &tevt_id, NULL);
	g_object_get (selected_evt, "summary", &tsummary_str, NULL);
	g_object_get (selected_evt, "location", &tlocation_str, NULL);
		
	db_update_event(selected_evt);
	
	m_row_index = -1;
	m_id_selection = -1;
	
	update_label_date(CUSTOM_CALENDAR(calendar), label_date);
	
	custom_calendar_reset_marks(CUSTOM_CALENDAR(calendar));		
	GArray *evt_arry_month; //add month marks
	evt_arry_month = g_array_new(FALSE, FALSE, sizeof(CALENDAR_TYPE_EVENT));
	db_get_all_events_year_month(evt_arry_month, m_start_year,m_start_month);
	
	set_marks_on_calendar(CUSTOM_CALENDAR(calendar),evt_arry_month);
	g_array_free(evt_arry_month, FALSE); //clear the array
		
	custom_calendar_update(CUSTOM_CALENDAR(calendar));
	
	GArray *evt_arry_day;	
	evt_arry_day = g_array_new(FALSE, FALSE, sizeof(CALENDAR_TYPE_EVENT)); // setup arraylist
	db_get_all_events_year_month_day(evt_arry_day, m_start_year,m_start_month, m_start_day);

	display_event_array(evt_arry_day);
	g_array_free(evt_arry_day, FALSE); //clear the array 
		
	gtk_window_destroy(GTK_WINDOW(dialog));
	
}
//======================================================================

static guint get_dropdown_position(const gchar* summary)
{
	
	guint position=0;
	gchar* summary_lower= g_ascii_strdown(summary,-1);
		
	if (g_strcmp0(summary_lower,"activity")==0) {
	position=0;
	}
	if (g_strcmp0(summary_lower,"anniversary")==0) {
	position=1;
	}	
	if (g_strcmp0(summary_lower,"appointment")==0) {
	position=2;
	}
	if (g_strcmp0(summary_lower,"birthday")==0) {
	position=3;
	}
		if (g_strcmp0(summary_lower,"cafe")==0) {
	position=4;
	}
	if (g_strcmp0(summary_lower,"car")==0) {
	position=5;
	}
	if (g_strcmp0(summary_lower,"dentist")==0) {
	position=6;
	}
	if (g_strcmp0(summary_lower,"doctor")==0) {
	position=7;
	}
	if (g_strcmp0(summary_lower,"driver")==0) {
	position=8;
	}
	if (g_strcmp0(summary_lower,"family")==0) {
	position=9;
	}	
	if (g_strcmp0(summary_lower,"funeral")==0) {
	position=10;
	}	
	if (g_strcmp0(summary_lower,"garden")==0) {
	position=11;
	}
	
	if (g_strcmp0(summary_lower,"holiday")==0) {
	position=12;
	}			
	if (g_strcmp0(summary_lower,"hospital")==0) {
	position=13;
	}	
	if (g_strcmp0(summary_lower,"meeting")==0) {
	position=14;
	}
	if (g_strcmp0(summary_lower,"meetup")==0) {
	position=15;
	}
	if (g_strcmp0(summary_lower,"party")==0) {
	position=16;
	}
	if (g_strcmp0(summary_lower,"payment")==0) {
	position=17;
	}
	if (g_strcmp0(summary_lower,"reminder")==0) {
	position=18;
	}
	if (g_strcmp0(summary_lower,"restaurant")==0) {
	position=19;
	}
	if (g_strcmp0(summary_lower,"task")==0) {
	position=20;
	}
	if (g_strcmp0(summary_lower,"travel")==0) {
	position=21;
	}
	if (g_strcmp0(summary_lower,"visit")==0) {
	position=22;
	}	
	if (g_strcmp0(summary_lower,"work")==0) {
	position=23;
	}
	
	return position;
}
//======================================================================
static void callbk_edit_event(GSimpleAction *action, GVariant *parameter,  gpointer user_data)
{	
	if (m_id_selection == -1)
		return;
	
	GtkWindow *window = user_data;

	GtkWidget *dialog;
	gint response;
	GtkWidget *grid;
	GtkWidget *box;
	GtkWidget *label_date;
	GtkEntryBuffer *buffer_summary;
	GtkEntryBuffer *buffer_location;
	GtkEntryBuffer *buffer_description; // placeholder

	GtkWidget *label_entry_summary;	
	GtkWidget *dropdown;

	GtkWidget *label_location;
	GtkWidget *entry_location;
	// Description
	GtkWidget *label_description;
	GtkWidget *entry_description;

	// Start time
	GtkWidget *label_start_time;
	GtkWidget *spin_button_start_time;
	GtkWidget *box_start_time;

	// End time
	GtkWidget *label_end_time;
	GtkWidget *spin_button_end_time;
	GtkWidget *box_end_time;

	// Check buttons
	GtkWidget *check_button_allday;
	GtkWidget *check_button_isyearly;
	GtkWidget *check_button_priority;
	GtkWidget *check_button_notification;

	GtkWidget *button_update;

	GDate *event_date;
	event_date = g_date_new();
	event_date = g_date_new_dmy(m_start_day, m_start_month, m_start_year);
	int event_day = g_date_get_day(event_date);
	int event_month = g_date_get_month(event_date);
	int event_year = g_date_get_year(event_date);

	gchar *date_str = "Event Date: ";
	gchar *day_str = g_strdup_printf("%d", event_day);
	gchar *month_str = g_strdup_printf("%d", event_month);
	gchar *year_str = g_strdup_printf("%d", event_year);

	date_str = g_strconcat(date_str, day_str, "-", month_str, "-", year_str, NULL);

	dialog = gtk_window_new(); // use this as gtk_dialog_new_with_buttons deprecated gtk4.10

	gtk_window_set_title(GTK_WINDOW(dialog), "Edit Event");
	gtk_window_set_default_size(GTK_WINDOW(dialog), 350, 100);

	box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 1);
	gtk_window_set_child(GTK_WINDOW(dialog), box);

	gint evt_id = 0;
	gchar *summary_str = "";
	gchar *location_str = "";
	gchar *description_str = "";

	gint start_year = 0;
	gint start_month = 0;
	gint start_day = 0;
	gint start_hour = 0;
	gint start_min = 0;

	gint end_year = 0;
	gint end_month = 0;
	gint end_day = 0;
	gint end_hour = 0;
	gint end_min = 0;

	gint is_yearly = 0;
	gint is_allday = 0;
	gint is_priority = 0;
	gint has_notification = 0;
	gint has_reminder = 0;
	gint reminder_min = 0;
	
	//if(selected_evt ==NULL) return; //to do
	
	g_object_get(selected_evt, "summary", &summary_str, NULL);
	g_object_get(selected_evt, "location", &location_str, NULL);
	g_object_get(selected_evt, "description", &description_str, NULL); // placeholders for future updates
	g_object_get (selected_evt, "startyear", &start_year, NULL);
	g_object_get (selected_evt, "startmonth", &start_month, NULL);
	g_object_get (selected_evt, "startday", &start_day, NULL);
	g_object_get(selected_evt, "starthour", &start_hour, NULL);
	g_object_get(selected_evt, "startmin", &start_min, NULL);
	g_object_get (selected_evt, "endyear", &end_year, NULL);
	g_object_get (selected_evt, "endmonth", &end_month, NULL);
	g_object_get (selected_evt, "endday", &end_day, NULL);
	g_object_get(selected_evt, "endhour", &end_hour, NULL);
	g_object_get(selected_evt, "endmin", &end_min, NULL);
	g_object_get(selected_evt, "isyearly", &is_yearly, NULL);
	g_object_get(selected_evt, "isallday", &is_allday, NULL);
	g_object_get(selected_evt, "ispriority", &is_priority, NULL);
	g_object_get (selected_evt, "hasnotification", &has_notification, NULL);	
	g_object_get (selected_evt, "hasreminder", &has_reminder, NULL);
	g_object_get (selected_evt, "remindermin", &reminder_min, NULL);
	
	m_summary = g_strdup(summary_str); //set m_summary
	m_location = g_strdup(location_str);
	m_description = g_strdup(description_str);
	m_start_year=start_year; //multiday not yet implemented
	m_start_month=start_month;
	m_start_day=start_day;	
	m_start_hour = start_hour;
	m_start_min = start_min;
	m_end_year=0; //multiday not yet implemented
	m_end_month=0;
	m_end_day=0;	
	m_end_hour = end_hour;
	m_end_min = end_min;	
	m_is_yearly = is_yearly;
	m_is_allday = is_allday;
	m_priority = is_priority;
	m_notification=has_notification;
	
	label_date = gtk_label_new(date_str);

	button_update = gtk_button_new_with_label("Update");
	g_signal_connect(button_update, "clicked", G_CALLBACK(callbk_update_event), window);

	label_entry_summary = gtk_label_new("Event Speech Word");
	dropdown =gtk_drop_down_new_from_strings(events);    
	g_signal_connect(GTK_DROP_DOWN(dropdown), "notify::selected", G_CALLBACK(callbk_dropdown), NULL);
	
	guint position=0;
	
	position = get_dropdown_position(m_summary);
	
	gtk_drop_down_set_selected(GTK_DROP_DOWN(dropdown),position);
  

	label_location = gtk_label_new("Location");
	entry_location = gtk_entry_new();
	gtk_entry_set_max_length(GTK_ENTRY(entry_location), 100);
	buffer_location = gtk_entry_buffer_new(m_location, -1); // show location
	gtk_entry_set_buffer(GTK_ENTRY(entry_location), buffer_location);

	//---------------------------------------------------------
	// Description entry
	//----------------------------------------------------------
	label_description = gtk_label_new("Description");
	entry_description = gtk_entry_new();
	gtk_entry_set_max_length(GTK_ENTRY(entry_description), 200);
	buffer_description = gtk_entry_buffer_new(m_description, -1); // show description
	gtk_entry_set_buffer(GTK_ENTRY(entry_description), buffer_description);

	gtk_box_append(GTK_BOX(box), label_date);
	gtk_box_append(GTK_BOX(box), label_entry_summary);	
	gtk_box_append(GTK_BOX(box), dropdown);
	
	gtk_box_append(GTK_BOX(box), label_description);
	gtk_box_append(GTK_BOX(box), entry_description);

	gtk_box_append(GTK_BOX(box), label_location);
	gtk_box_append(GTK_BOX(box), entry_location);

	g_object_set_data(G_OBJECT(button_update), "dialog-key", dialog);
	
	g_object_set_data(G_OBJECT(button_update), "entry-location-key", entry_location);
	g_object_set_data(G_OBJECT(button_update), "dialog-window-key", window);
	g_object_set_data(G_OBJECT(button_update), "entry-description-key", entry_description);

	// start time spin
	GtkAdjustment *adjustment_start;
	adjustment_start = gtk_adjustment_new(8.00, 0.0, 23.59, 1.0, 1.0, 0.0);
	// start time spin
	label_start_time = gtk_label_new("Start Time (24 hour) ");
	spin_button_start_time = gtk_spin_button_new(adjustment_start, 1.0, 2);

	// convert integer hour-min values to float
	// double d = wholeNumber + (double)decimal / 100; //todo change floats to doubles

	m_start_time = m_start_hour + (float)m_start_min / 100;

	gtk_spin_button_set_value(GTK_SPIN_BUTTON(spin_button_start_time), m_start_time);
	box_start_time = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 1);
	gtk_box_append(GTK_BOX(box_start_time), label_start_time);
	gtk_box_append(GTK_BOX(box_start_time), spin_button_start_time);
	gtk_box_append(GTK_BOX(box), box_start_time);

	g_object_set_data(G_OBJECT(button_update), "spin-start-time-key", spin_button_start_time);

	// end time spin
	GtkAdjustment *adjustment_end;
	// value,lower,upper,step_increment,page_increment,page_size
	adjustment_end = gtk_adjustment_new(8.00, 0.0, 23.59, 1.0, 1.0, 0.0);
	label_end_time = gtk_label_new("End Time (24 hour) ");
	spin_button_end_time = gtk_spin_button_new(adjustment_end, 1.0, 2);

	m_end_time = m_end_hour + (float)m_end_min / 100;
	gtk_spin_button_set_value(GTK_SPIN_BUTTON(spin_button_end_time), m_end_time);
	box_end_time = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 1);

	gtk_box_append(GTK_BOX(box_end_time), label_end_time);
	gtk_box_append(GTK_BOX(box_end_time), spin_button_end_time);
	gtk_box_append(GTK_BOX(box), box_end_time);

	g_object_set_data(G_OBJECT(button_update), "spin-end-time-key", spin_button_end_time);

	// check buttons
	check_button_allday = gtk_check_button_new_with_label("Is All Day");
	
	g_object_set_data(G_OBJECT(check_button_allday), "cb_allday_spin_start_time_key", spin_button_start_time);
	g_object_set_data(G_OBJECT(check_button_allday), "cb_allday_spin_end_time_key", spin_button_end_time);

	g_signal_connect_swapped(GTK_CHECK_BUTTON(check_button_allday), "toggled",
							 G_CALLBACK(callbk_check_button_allday_toggled), check_button_allday);

	if (m_is_allday)
	{
		gtk_widget_set_sensitive(spin_button_start_time, FALSE);
		gtk_widget_set_sensitive(spin_button_end_time, FALSE);
	}
	else
	{
		gtk_widget_set_sensitive(spin_button_start_time, TRUE);
		gtk_widget_set_sensitive(spin_button_end_time, TRUE);
	}

	check_button_isyearly = gtk_check_button_new_with_label("Is Yearly");
	check_button_priority = gtk_check_button_new_with_label("Is High Priority");
	check_button_notification = gtk_check_button_new_with_label("Send Notification");
	
	gtk_check_button_set_active(GTK_CHECK_BUTTON(check_button_isyearly), m_is_yearly);
	gtk_check_button_set_active(GTK_CHECK_BUTTON(check_button_allday), m_is_allday);
	gtk_check_button_set_active(GTK_CHECK_BUTTON(check_button_priority), m_priority);
	gtk_check_button_set_active(GTK_CHECK_BUTTON(check_button_notification), m_notification);
	
	gtk_box_append(GTK_BOX(box), check_button_allday);
	gtk_box_append(GTK_BOX(box), check_button_isyearly);
	gtk_box_append(GTK_BOX(box), check_button_priority);
	gtk_box_append(GTK_BOX(box), check_button_notification);
	gtk_box_append(GTK_BOX(box), button_update);
	
	g_object_set_data(G_OBJECT(button_update), "check-button-allday-key", check_button_allday);
	g_object_set_data(G_OBJECT(button_update), "check-button-isyearly-key", check_button_isyearly);
	g_object_set_data(G_OBJECT(button_update), "check-button-priority-key", check_button_priority);
	g_object_set_data(G_OBJECT(button_update), "check-button-notification-key", check_button_notification);

	gtk_window_present(GTK_WINDOW(dialog));
}
//======================================================================

static void callbk_delete_selected(GSimpleAction *action, GVariant *parameter,  gpointer user_data)
{
	GtkWindow *window =user_data;
	GtkWidget *calendar =g_object_get_data(G_OBJECT(window), "window-calendar-key");
	GtkWidget *label_date =g_object_get_data(G_OBJECT(window), "window-label-date-key");
	int selected_evt_id=0;
	g_object_get (selected_evt, "eventid", &selected_evt_id, NULL);	
	
	db_delete_row(selected_evt_id);
	m_row_index=-1; //used for delete selection
	m_id_selection=-1;
	
	update_label_date(CUSTOM_CALENDAR(calendar), label_date);
	
	custom_calendar_reset_marks(CUSTOM_CALENDAR(calendar));		
	
	//update month events
	GArray *evt_arry_month; //add month marks
	evt_arry_month = g_array_new(FALSE, FALSE, sizeof(CALENDAR_TYPE_EVENT));
	db_get_all_events_year_month(evt_arry_month, m_start_year,m_start_month);	
	set_marks_on_calendar(CUSTOM_CALENDAR(calendar),evt_arry_month);
	g_array_free(evt_arry_month, FALSE); //clear the array
	
	//update day events	
	g_list_store_remove_all(m_store); // clear
	
	GArray *evt_arry_day;	
	evt_arry_day = g_array_new(FALSE, FALSE, sizeof(CALENDAR_TYPE_EVENT)); // setup arraylist
	db_get_all_events_year_month_day(evt_arry_day, m_start_year,m_start_month, m_start_day);	
	display_event_array(evt_arry_day);
	g_array_free(evt_arry_day, FALSE); //clear the array 
	
	//update calendar	
	custom_calendar_update(CUSTOM_CALENDAR(calendar));	
}

//======================================================================
static void callbk_confirm_delete_all(GtkButton *button, gpointer  user_data)
{	
	GtkWindow *window =user_data;	
	GtkWidget *calendar =g_object_get_data(G_OBJECT(window), "window-calendar-key");
	GtkWidget *label_date =g_object_get_data(G_OBJECT(window), "window-label-date-key");
	
	
	GtkWidget *dialog = g_object_get_data(G_OBJECT(button), "dialog-key");
	
	db_delete_all();
	
	g_list_store_remove_all(m_store); // clear store
	
	custom_calendar_reset_marks(CUSTOM_CALENDAR(calendar));		
			
	custom_calendar_update(CUSTOM_CALENDAR(calendar));
	gtk_window_destroy(GTK_WINDOW(dialog));		
}
//======================================================================
static void callbk_delete_all(GSimpleAction *action, GVariant *parameter,  gpointer user_data)
{
	GtkWidget *window =user_data;

	GtkWidget *dialog;
	GtkWidget *box;
	GtkWidget *button_confirm;
	GtkWidget *label_confirm;
	
	dialog =gtk_window_new(); //gtk_dialog_new_with_buttons to be deprecated gtk4.10

	gtk_window_set_title (GTK_WINDOW (dialog), "Delete All");
	gtk_window_set_default_size(GTK_WINDOW(dialog),350,100);

	box =gtk_box_new(GTK_ORIENTATION_VERTICAL,1);
	gtk_window_set_child (GTK_WINDOW (dialog), box);

	button_confirm = gtk_button_new_with_label ("Delete All");
	g_signal_connect (button_confirm, "clicked", G_CALLBACK (callbk_confirm_delete_all), window);
	
	label_confirm = gtk_label_new("Pressing Delete All\n will clear database");
	
	g_object_set_data(G_OBJECT(button_confirm), "dialog-key",dialog);
	
	gtk_box_append(GTK_BOX(box), label_confirm);	
	gtk_box_append(GTK_BOX(box), button_confirm);
	gtk_window_present (GTK_WINDOW (dialog));	
	
}

//======================================================================
// calendar functions
//======================================================================

static void set_holidays_on_calendar(CustomCalendar *calendar){
	
	custom_calendar_reset_holidays(CUSTOM_CALENDAR(calendar));	
	
	guint8 month_days =g_date_get_days_in_month(m_start_month,m_start_year);	
		
	for (int day=1; day<=month_days; day++)
	{
		if (is_notable_date(day))
		{
			custom_calendar_mark_holiday(CUSTOM_CALENDAR(calendar),day);
		}
	}
}


//======================================================================
//Implementation of Calendar menu callbks
//======================================================================

//======================================================================
// Callback home (go to current date)
//======================================================================
static void callbk_calendar_home(GSimpleAction * action, GVariant *parameter, gpointer user_data)
{
	GtkWindow *window =user_data;	
	GtkWidget *calendar =g_object_get_data(G_OBJECT(window), "window-calendar-key");
	GtkWidget *label_date =g_object_get_data(G_OBJECT(window), "window-label-date-key");
	
	custom_calendar_goto_today(CUSTOM_CALENDAR(calendar));
	
	m_today_day = custom_calendar_get_day(CUSTOM_CALENDAR(calendar));
	m_today_month = custom_calendar_get_month(CUSTOM_CALENDAR(calendar));
	m_today_year = custom_calendar_get_year(CUSTOM_CALENDAR(calendar));		
	
	m_start_day=m_today_day;
	m_start_month=m_today_month;
	m_start_year=m_today_year;
	
	m_id_selection = -1;	
	
	//mark up calendar	
	if(m_holidays) set_holidays_on_calendar(CUSTOM_CALENDAR(calendar));
	update_label_date(CUSTOM_CALENDAR(calendar), label_date);	
	
	GArray *evt_arry_month; //add month marks
	evt_arry_month = g_array_new(FALSE, FALSE, sizeof(CALENDAR_TYPE_EVENT));
	db_get_all_events_year_month(evt_arry_month, m_start_year,m_start_month);
	
	set_marks_on_calendar(CUSTOM_CALENDAR(calendar),evt_arry_month);
	g_array_free(evt_arry_month, FALSE); //clear the array
	
	GArray *evt_arry_day;
	
	evt_arry_day = g_array_new(FALSE, FALSE, sizeof(CALENDAR_TYPE_EVENT)); // setup arraylist
	db_get_all_events_year_month_day(evt_arry_day, m_start_year,m_start_month, m_start_day);
		
	display_event_array(evt_arry_day);
	g_array_free(evt_arry_day, FALSE); //clear the array 
	
	custom_calendar_update(CUSTOM_CALENDAR(calendar));		
}
//======================================================================

gboolean is_colour_name(gchar* colour_name)
{
	gboolean found =FALSE;
	
	GList* list = NULL;
	//reds
	list = g_list_append(list, "indianred");	
	list = g_list_append(list, "lightcoral");
	list = g_list_append(list, "salmon");
	list = g_list_append(list, "darksalmon");
	list = g_list_append(list, "lightsalmon ");
	list = g_list_append(list, "crimson");	 	
	list = g_list_append(list, "red");	
	list = g_list_append(list, "firebrick");
	list = g_list_append(list, "darkred");
	//pinks
	list = g_list_append(list,"pink");
	list = g_list_append(list,"lightpink");	
	list = g_list_append(list,"hotpink");
	list = g_list_append(list,"deeppink");	
	list = g_list_append(list,"mediumvioletred");
	list = g_list_append(list,"palevioletred");
	//oranges
	list = g_list_append(list,"lightsalmon"); 
	list = g_list_append(list,"coral"); 
	list = g_list_append(list,"tomato"); 
	list = g_list_append(list,"orangered"); 
	list = g_list_append(list,"darkorange"); 
	list = g_list_append(list,"orange"); 
	//yellows
	list = g_list_append(list,"gold"); 	
	list = g_list_append(list,"yellow"); 	
	list = g_list_append(list,"lightyellow"); 	
	list = g_list_append(list,"lemonchiffon"); 	
	list = g_list_append(list,"lightgoldenrodyellow"); 	
	list = g_list_append(list,"papayawhip"); 	
	list = g_list_append(list,"moccasin"); 	
	list = g_list_append(list,"peachpuff"); 	
	list = g_list_append(list,"palegoldenrod"); 	
	list = g_list_append(list,"khaki"); 	
	list = g_list_append(list,"darkkhaki"); 	
	//purples	
	list = g_list_append(list,"lavender"); 
	list = g_list_append(list,"thistle"); 
	list = g_list_append(list,"plum"); 
	list = g_list_append(list,"violet"); 
	list = g_list_append(list,"orchid"); 
	list = g_list_append(list,"fuchsia"); 
	list = g_list_append(list,"magenta");	
	list = g_list_append(list,"mediumorchid");
	list = g_list_append(list,"mediumpurple");
	list = g_list_append(list,"rebeccapurple");
	list = g_list_append(list,"blueviolet");
	list = g_list_append(list,"darkviolet");
	list = g_list_append(list,"darkorchid");
	list = g_list_append(list,"darkmagenta");	 	
	list = g_list_append(list,"purple"); 
	list = g_list_append(list,"indigo"); 
	list = g_list_append(list,"slateblue"); 
	list = g_list_append(list,"darkslateblue"); 
	list = g_list_append(list,"mediumslateblue"); 
	//greens	
	list = g_list_append(list, "greenyellow");
	list = g_list_append(list, "chartreuse");
	list = g_list_append(list, "lawngreen");
	list = g_list_append(list, "lime");
	list = g_list_append(list, "limegreen");
	list = g_list_append(list, "palegreen");
	list = g_list_append(list, "lightgreen");
	list = g_list_append(list, "mediumspringgreen");
	list = g_list_append(list, "springgreen");
	list = g_list_append(list, "mediumseagreen");
	list = g_list_append(list, "seagreen");
	list = g_list_append(list, "forestgreen");	
	list = g_list_append(list, "green");
	list = g_list_append(list,"darkgreen");
	list = g_list_append(list, "yellowgreen");
	list = g_list_append(list, "olivedrab");
	list = g_list_append(list,"olive");
	list = g_list_append(list, "darkolivegreen");
	list = g_list_append(list, "mediumaquamarine");
	list = g_list_append(list, "darkseagreen");
	list = g_list_append(list, "lightseagreen");
	list = g_list_append(list, "darkcyan");
	list = g_list_append(list, "teal");
	//blues	
	list = g_list_append(list,"aqua"); 
	list = g_list_append(list,"cyan"); 	
	list = g_list_append(list,"lightcyan");
	list = g_list_append(list,"paleturquoise");
	list = g_list_append(list,"aquamarine");
	list = g_list_append(list,"turquoise");
	list = g_list_append(list,"mediumturquoise");
	list = g_list_append(list,"darkturquoise");
	list = g_list_append(list,"cadetblue");
	list = g_list_append(list,"steelblue");
	list = g_list_append(list,"lightsteelblue");
	list = g_list_append(list,"powderblue");
	list = g_list_append(list,"lightblue"); 
	list = g_list_append(list,"skyblue");
	list = g_list_append(list,"lightskyblue");
	list = g_list_append(list,"deepskyblue");
	list = g_list_append(list,"dodgerblue");
	list = g_list_append(list,"cornflowerblue");
	list = g_list_append(list,"mediumslateblue");
	list = g_list_append(list,"royalblue");
	list = g_list_append(list, "blue");	
	list = g_list_append(list, "mediumblue");
	list = g_list_append(list,"darkblue");
	list = g_list_append(list, "navy");	
	list = g_list_append(list, "midnightblue");	
	//browns	
	list = g_list_append(list,"cornsilk");
	list = g_list_append(list,"blanchedalmond"); 
	list = g_list_append(list,"navajowhite"); 
	list = g_list_append(list,"wheat"); 
	list = g_list_append(list,"burlywood"); 
	list = g_list_append(list,"tan"); 
	list = g_list_append(list,"rosybrown"); 
	list = g_list_append(list,"sandybrown"); 
	list = g_list_append(list,"goldenrod"); 
	list = g_list_append(list,"darkgoldenrod"); 
	list = g_list_append(list,"peru"); 
	list = g_list_append(list,"chocolate"); 
	list = g_list_append(list,"saddlebrown"); 
	list = g_list_append(list,"sienna"); 	
	list = g_list_append(list,"brown");
	list = g_list_append(list,"maroon");  	
	//whites
	list = g_list_append(list,"white");	
	list = g_list_append(list,"snow");
	list = g_list_append(list,"honeydew");
	list = g_list_append(list,"mintcream");
	list = g_list_append(list,"azure");
	list = g_list_append(list,"aliceblue");
	list = g_list_append(list,"ghostwhite");
	list = g_list_append(list,"whitesmoke");
	list = g_list_append(list,"seashell");
	list = g_list_append(list,"beige");
	list = g_list_append(list,"oldlace");
	list = g_list_append(list,"floralwhite");
	list = g_list_append(list,"ivory");
	list = g_list_append(list,"antiquewhite");
	list = g_list_append(list,"linen");
	list = g_list_append(list,"lavenderblush");
	list = g_list_append(list,"mistyrose");
	//greys			
	list = g_list_append(list,"gainsboro"); 
	list = g_list_append(list,"lightgray");		
	list = g_list_append(list,"silver"); 
	list = g_list_append(list,"darkgray");	
	list = g_list_append(list,"gray"); 
	list = g_list_append(list,"dimgray");
	list = g_list_append(list,"lightslategray");
	list = g_list_append(list,"slategray");
	list = g_list_append(list,"darkslategray");
	list = g_list_append(list,"black");
		
	gpointer pt_data;
	gchar* list_str;	
	
	for(int i=0;i<g_list_length(list);i++) //iterate through GList 
	{	  
		pt_data=g_list_nth_data(list,i);
		list_str=(char *)pt_data;		
		
		if (g_strcmp0(colour_name,list_str)==0) {
			found =TRUE;
		}	
	}
	g_list_free(list);
	return found;
}
//======================================================================

static void callbk_update_colours(GtkButton *button, gpointer user_data)
{
	GtkWidget *window = user_data;
	GtkWidget *calendar = g_object_get_data(G_OBJECT(window), "window-calendar-key");
	GtkWidget *label_date = g_object_get_data(G_OBJECT(window), "window-label-date-key");
	GtkWidget *dialog = g_object_get_data(G_OBJECT(button), "dialog-key");
	
	GtkEntryBuffer *buffer_todaycolour;
	GtkWidget *entry_todaycolour = g_object_get_data(G_OBJECT(button), "entry-todaycolour-key");
	
	GtkEntryBuffer *buffer_eventcolour;
	GtkWidget *entry_eventcolour = g_object_get_data(G_OBJECT(button), "entry-eventcolour-key");
	
	GtkEntryBuffer *buffer_holidaycolour;
	GtkWidget *entry_holidaycolour = g_object_get_data(G_OBJECT(button), "entry-holidaycolour-key");
	
	buffer_todaycolour = gtk_entry_get_buffer(GTK_ENTRY(entry_todaycolour));
	const gchar *todaycolour = gtk_entry_buffer_get_text(buffer_todaycolour);	
	gchar* todaycolour_lower= g_ascii_strdown(todaycolour,-1);
	//remove unwanted chars?	
	if(is_colour_name(todaycolour_lower)) m_todaycolour =todaycolour_lower;
     	
	buffer_eventcolour = gtk_entry_get_buffer(GTK_ENTRY(entry_eventcolour));
	const gchar* eventcolour = gtk_entry_buffer_get_text(buffer_eventcolour);
	gchar* eventcolour_lower=g_ascii_strdown(eventcolour,-1);
	if(is_colour_name(eventcolour_lower)) m_eventcolour =eventcolour_lower;
	
	buffer_holidaycolour = gtk_entry_get_buffer(GTK_ENTRY(entry_holidaycolour));
	const gchar* holidaycolour = gtk_entry_buffer_get_text(buffer_holidaycolour);
	gchar* holidaycolour_lower=g_ascii_strdown(holidaycolour,-1);
	if(is_colour_name(holidaycolour_lower)) m_holidaycolour =holidaycolour_lower;
		
	g_object_set(calendar, "todaycolour", m_todaycolour, NULL);
	g_object_set(calendar, "eventcolour", m_eventcolour, NULL);
	g_object_set(calendar, "holidaycolour", m_holidaycolour, NULL);	
	custom_calendar_update(CUSTOM_CALENDAR(calendar));
		
	//save preferences
	config_write();
		
	gtk_window_destroy(GTK_WINDOW(dialog));
}
//======================================================================
static void callbk_calendar_colour(GSimpleAction * action, GVariant *parameter, gpointer user_data)
{	
	GtkWidget *window =user_data;
	GtkWidget *dialog;
	GtkWidget *button_update;
	GtkWidget *box;
	
	GtkWidget *label_colour_info;
	
	GtkWidget *label_entry_todaycolour;
	GtkWidget *entry_todaycolour;
	GtkEntryBuffer *buffer_todaycolour;
	
	GtkWidget *label_entry_eventcolour;
	GtkWidget *entry_eventcolour;
	GtkEntryBuffer *buffer_eventcolour;
	
	GtkWidget *label_entry_holidaycolour;
	GtkWidget *entry_holidaycolour;	
	GtkEntryBuffer *buffer_holidaycolour; 
	
	dialog = gtk_window_new(); // gtk_dialog_new_with_buttons to be deprecated gtk4.10

	gtk_window_set_title(GTK_WINDOW(dialog), "Calendar Colours");
	gtk_window_set_default_size(GTK_WINDOW(dialog), 350, 100);

	box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 1);
	gtk_window_set_child(GTK_WINDOW(dialog), box);

	button_update = gtk_button_new_with_label("Update Colours");
	g_signal_connect(button_update, "clicked", G_CALLBACK(callbk_update_colours), window);
	
	label_colour_info=gtk_label_new("Use HTML colour names");
		
	label_entry_todaycolour = gtk_label_new("Today Colour");
	entry_todaycolour = gtk_entry_new();
	gtk_entry_set_max_length(GTK_ENTRY(entry_todaycolour), 50);
	buffer_todaycolour = gtk_entry_buffer_new(m_todaycolour, -1); // show current colour
	gtk_entry_set_buffer(GTK_ENTRY(entry_todaycolour), buffer_todaycolour);
		
	label_entry_eventcolour = gtk_label_new("Event Colour");
	entry_eventcolour = gtk_entry_new();
	gtk_entry_set_max_length(GTK_ENTRY(entry_eventcolour), 50);
	buffer_eventcolour = gtk_entry_buffer_new(m_eventcolour, -1); // show current colour
	gtk_entry_set_buffer(GTK_ENTRY(entry_eventcolour), buffer_eventcolour);
	
	label_entry_holidaycolour = gtk_label_new("Holiday Colour");
	entry_holidaycolour = gtk_entry_new();
	gtk_entry_set_max_length(GTK_ENTRY(entry_holidaycolour), 50);
	buffer_holidaycolour = gtk_entry_buffer_new(m_holidaycolour, -1); // show current colour
	gtk_entry_set_buffer(GTK_ENTRY(entry_holidaycolour), buffer_holidaycolour);
	
	
	gtk_box_append(GTK_BOX(box), label_colour_info);
	gtk_box_append(GTK_BOX(box), label_entry_todaycolour);
	gtk_box_append(GTK_BOX(box), entry_todaycolour);
	gtk_box_append(GTK_BOX(box), label_entry_eventcolour);
	gtk_box_append(GTK_BOX(box), entry_eventcolour);
	gtk_box_append(GTK_BOX(box), label_entry_holidaycolour);
	gtk_box_append(GTK_BOX(box), entry_holidaycolour);
	gtk_box_append(GTK_BOX(box), button_update);
	
	g_object_set_data(G_OBJECT(button_update), "entry-todaycolour-key", entry_todaycolour);
	g_object_set_data(G_OBJECT(button_update), "entry-eventcolour-key", entry_eventcolour);	
	g_object_set_data(G_OBJECT(button_update), "entry-holidaycolour-key", entry_holidaycolour);
	g_object_set_data(G_OBJECT(button_update), "dialog-key", dialog);
	
	gtk_window_present(GTK_WINDOW(dialog));	
	
}
//======================================================================
//======================================================================
// Update Date Label
//======================================================================

static void update_label_date(CustomCalendar *calendar, gpointer user_data)
{
GtkWidget *label_date = (GtkWidget *) user_data;
	
	gchar* date_str="";
	 gchar* weekday_str="";

	 GDateTime *dt;
	 dt = g_date_time_new_local(m_start_year, m_start_month, m_start_day, 1, 1, 1);
	 gint day_of_week = g_date_time_get_day_of_week(dt);
	 g_date_time_unref(dt); //freeit quick

	 switch(day_of_week)
	 {
	 	case G_DATE_MONDAY:
	 		weekday_str="Monday";
	 		break;
	 	case G_DATE_TUESDAY:
	 		weekday_str="Tuesday";
	 		break;
	 	case G_DATE_WEDNESDAY:
	 		weekday_str="Wednesday";
	 		break;
	 	case G_DATE_THURSDAY:
	 		weekday_str="Thursday";
	 		break;
	 	case G_DATE_FRIDAY:
	 		weekday_str="Friday";
	 		break;
	 	case G_DATE_SATURDAY:
	 		weekday_str="Saturday";
	 		break;
	 	case G_DATE_SUNDAY:
	 		weekday_str="Sunday";
	 		break;
	 	default:
	 		weekday_str="Unknown";
	 }//switch
	 
	 gchar* day_str =  g_strdup_printf("%d",m_start_day);
	 gchar *year_str = g_strdup_printf("%d",m_start_year);

	date_str =g_strconcat(date_str,weekday_str," ", day_str, " ", NULL);

	 switch(m_start_month)
	 {
	 	case G_DATE_JANUARY:
	 		date_str =g_strconcat(date_str,"January ",year_str, NULL);
	 		break;
	 	case G_DATE_FEBRUARY:
	 		date_str =g_strconcat(date_str,"February ",year_str, NULL);
	 		break;
	 	case G_DATE_MARCH:
	 		date_str =g_strconcat(date_str,"March ",year_str, NULL);
	 		break;
	 	case G_DATE_APRIL:
	 		date_str =g_strconcat(date_str,"April ",year_str, NULL);
	 		break;
	 	case G_DATE_MAY:
	 		date_str =g_strconcat(date_str,"May ",year_str, NULL);
	 		break;
	 	case G_DATE_JUNE:
	 		date_str =g_strconcat(date_str,"June ",year_str, NULL);
	 		break;
	 	case G_DATE_JULY:
	 		date_str =g_strconcat(date_str,"July ",year_str, NULL);
	 		break;
	 	case G_DATE_AUGUST:
	 		date_str =g_strconcat(date_str,"August ",year_str, NULL);
	 		break;
	 	case G_DATE_SEPTEMBER:
	 		date_str =g_strconcat(date_str,"September ",year_str, NULL);
	 		break;
	 	case G_DATE_OCTOBER:
	 		date_str =g_strconcat(date_str,"October ",year_str, NULL);
	 		break;
	 	case G_DATE_NOVEMBER:
	 		date_str =g_strconcat(date_str,"November ",year_str, NULL);
	 		break;
	 	case G_DATE_DECEMBER:
	 		date_str =g_strconcat(date_str,"December ",year_str, NULL);
	 		break;
	 	default:
	 		date_str =g_strconcat(date_str,"Unknown ",year_str, NULL);
	 }
		
	if ((m_holidays ==1) && (is_notable_date(m_start_day)))
	{
		
		gchar * holiday_str = get_notable_date_str(m_start_day);	 	
	 	date_str =g_strconcat(date_str," ",holiday_str, NULL);
	}

	
	 int event_num =get_number_of_day_events();
	 if(event_num>0) {
	 	date_str =g_strconcat(date_str,"*", NULL);
	 }
	 
	gtk_label_set_text(GTK_LABEL(label_date), date_str);

	
	 
}
//======================================================================
// Calendar callbks
//======================================================================

static void callbk_calendar_next_month(CustomCalendar *calendar, gpointer user_data) 
{
	GtkWidget *label_date = (GtkWidget *)user_data;
	
	m_start_day = custom_calendar_get_day(CUSTOM_CALENDAR(calendar));
	m_start_month = custom_calendar_get_month(CUSTOM_CALENDAR(calendar));
	m_start_year = custom_calendar_get_year(CUSTOM_CALENDAR(calendar));	
	
	g_list_store_remove_all(m_store); // clear listbox store
	
	//mark up calendar	
	if(m_holidays) set_holidays_on_calendar(CUSTOM_CALENDAR(calendar));
	
	update_label_date(CUSTOM_CALENDAR(calendar), label_date);	
		
	GArray *evt_arry_month; //add month marks
	evt_arry_month = g_array_new(FALSE, FALSE, sizeof(CALENDAR_TYPE_EVENT));
	db_get_all_events_year_month(evt_arry_month, m_start_year,m_start_month);	
	set_marks_on_calendar(CUSTOM_CALENDAR(calendar),evt_arry_month);
	g_array_free(evt_arry_month, FALSE); //clear the array
	
	custom_calendar_update(CUSTOM_CALENDAR(calendar));	
}
//======================================================================
static void callbk_calendar_prev_month(CustomCalendar *calendar, gpointer user_data) 
{
	
	GtkWidget *label_date = (GtkWidget *)user_data;	
	m_start_day = custom_calendar_get_day(CUSTOM_CALENDAR(calendar));
	m_start_month = custom_calendar_get_month(CUSTOM_CALENDAR(calendar));
	m_start_year = custom_calendar_get_year(CUSTOM_CALENDAR(calendar));	
		
	g_list_store_remove_all(m_store); // clear listbox store
	
	//mark up calendar	
	if(m_holidays) set_holidays_on_calendar(CUSTOM_CALENDAR(calendar));
	
	update_label_date(CUSTOM_CALENDAR(calendar), label_date);	
			
	GArray *evt_arry_month; //add month marks
	evt_arry_month = g_array_new(FALSE, FALSE, sizeof(CALENDAR_TYPE_EVENT));
	db_get_all_events_year_month(evt_arry_month, m_start_year,m_start_month);	
	set_marks_on_calendar(CUSTOM_CALENDAR(calendar),evt_arry_month);
	g_array_free(evt_arry_month, FALSE); //clear the array
		
	custom_calendar_update(CUSTOM_CALENDAR(calendar));
}
//======================================================================
static void callbk_calendar_next_year(CustomCalendar *calendar, gpointer user_data) 
{
	GtkWidget *label_date = (GtkWidget *)user_data;
	
	m_start_day = custom_calendar_get_day(CUSTOM_CALENDAR(calendar));
	m_start_month = custom_calendar_get_month(CUSTOM_CALENDAR(calendar));
	m_start_year = custom_calendar_get_year(CUSTOM_CALENDAR(calendar));	
	
	g_list_store_remove_all(m_store); // clear
	
	//mark up calendar	
	if(m_holidays) set_holidays_on_calendar(CUSTOM_CALENDAR(calendar));
	
	update_label_date(CUSTOM_CALENDAR(calendar), label_date);	
	
	GArray *evt_arry_month; //add month marks
	evt_arry_month = g_array_new(FALSE, FALSE, sizeof(CALENDAR_TYPE_EVENT));
	db_get_all_events_year_month(evt_arry_month, m_start_year,m_start_month);	
	set_marks_on_calendar(CUSTOM_CALENDAR(calendar),evt_arry_month);
	g_array_free(evt_arry_month, FALSE); //clear the array
	custom_calendar_update(CUSTOM_CALENDAR(calendar));
	
}
//======================================================================
static void callbk_calendar_prev_year(CustomCalendar *calendar, gpointer user_data) 
{
	GtkWidget *label_date = (GtkWidget *)user_data;
	
	m_start_day = custom_calendar_get_day(CUSTOM_CALENDAR(calendar));
	m_start_month = custom_calendar_get_month(CUSTOM_CALENDAR(calendar));
	m_start_year = custom_calendar_get_year(CUSTOM_CALENDAR(calendar));	
	
	g_list_store_remove_all(m_store); // clear
	
	//mark up calendar	
	if(m_holidays) set_holidays_on_calendar(CUSTOM_CALENDAR(calendar));
	
	update_label_date(CUSTOM_CALENDAR(calendar), label_date);	
			
	GArray *evt_arry_month; //add month marks
	evt_arry_month = g_array_new(FALSE, FALSE, sizeof(CALENDAR_TYPE_EVENT));
	db_get_all_events_year_month(evt_arry_month, m_start_year,m_start_month);	
	set_marks_on_calendar(CUSTOM_CALENDAR(calendar),evt_arry_month);
	g_array_free(evt_arry_month, FALSE); //clear the array
	
	custom_calendar_update(CUSTOM_CALENDAR(calendar));	
}
//======================================================================

static void callbk_calendar_day_selected(CustomCalendar *calendar, gpointer user_data)
{
	GtkWidget *label_date = (GtkWidget *)user_data;
	
	m_start_day = custom_calendar_get_day(CUSTOM_CALENDAR(calendar));
	m_start_month = custom_calendar_get_month(CUSTOM_CALENDAR(calendar));
	m_start_year = custom_calendar_get_year(CUSTOM_CALENDAR(calendar));
			
	g_list_store_remove_all(m_store); // clear listbox store
	
	//mark up calendar	
	if(m_holidays) set_holidays_on_calendar(CUSTOM_CALENDAR(calendar));
	
	update_label_date(CUSTOM_CALENDAR(calendar), label_date);		
	
	GArray *evt_arry_day; //normal day events	
	evt_arry_day = g_array_new(FALSE, FALSE, sizeof(CALENDAR_TYPE_EVENT)); // setup arraylist
	db_get_all_events_year_month_day(evt_arry_day, m_start_year,m_start_month, m_start_day);
	display_event_array(evt_arry_day); //display listbox day events
	g_array_free(evt_arry_day, FALSE); //clear the array 
	
	custom_calendar_update(CUSTOM_CALENDAR(calendar));				
}

//======================================================================
// calendar functions
//======================================================================

static void set_marks_on_calendar(CustomCalendar *calendar, GArray *arry) 
{	
	//custom_calendar_mark_day(CUSTOM_CALENDAR(calendar), 1);//testing
	custom_calendar_reset_marks(CUSTOM_CALENDAR(calendar));		
	for (int i=0; i<arry->len; i++) {  
		gint start_day=0;
		CalendarEvent *evt =g_array_index(arry, CalendarEvent*, i);
		g_object_get (evt, "startday", &start_day, NULL);
		custom_calendar_mark_day(CUSTOM_CALENDAR(calendar), start_day);	
	} //for evt_arry_month
}
//======================================================================
//======================================================================
//Implementation of Help menu callbks
//======================================================================

static void callbk_info(GSimpleAction *action, GVariant *parameter,  gpointer user_data)
{	
	
	

	
	GtkWidget *window =user_data;
	GtkWidget *dialog;
	GtkWidget *box;
	gint response;
	
	GtkWidget *label_keyboard_shortcuts;
	GtkWidget *label_speak_shortcut;
	GtkWidget *label_home_shortcut;
	GtkWidget *label_newevent_shortcut;
	GtkWidget *label_editevent_shortcut;	
	GtkWidget *label_deleteevent_shortcut;
	GtkWidget *label_preferences_shortcut;
	GtkWidget *label_time_shortcut;
		
	GtkWidget *label_record_info;
	GtkWidget *label_record_number;
	GtkWidget *label_sqlite_version;
	
	GtkWidget *label_font_info;	
	GtkWidget *label_desktop_font;
	GtkWidget *label_gnome_text_scale;
	
	GSettings *settings;
	
	PangoAttrList *attrs;
	attrs = pango_attr_list_new();	 
	pango_attr_list_insert(attrs, pango_attr_weight_new(PANGO_WEIGHT_BOLD));
		
    //replaced dialog with window as gtk_dialog_new_with_buttons to be deprecated gtk4.10
	dialog =gtk_window_new();
	
	gtk_window_set_default_size(GTK_WINDOW(dialog),380,100);
	gtk_window_set_title (GTK_WINDOW (dialog), "Information");
	
	box =gtk_box_new(GTK_ORIENTATION_VERTICAL,1);
	gtk_window_set_child (GTK_WINDOW (dialog), box);
	
	label_keyboard_shortcuts=gtk_label_new("Keyboard Shortcuts");
	gtk_label_set_attributes (GTK_LABEL (label_keyboard_shortcuts), attrs);
		
	label_home_shortcut=gtk_label_new("Home: Go to today");
	label_newevent_shortcut=gtk_label_new("Ctrl+e: New event");
	label_editevent_shortcut=gtk_label_new("Ctrl+n: Edit selected event");	
	label_deleteevent_shortcut=gtk_label_new("Delete: Delete selected event");
	label_preferences_shortcut=gtk_label_new("Ctrl+Alt+P: Preferences");
	label_speak_shortcut=gtk_label_new("Spacebar: Speak");
	label_time_shortcut=gtk_label_new("t: Speak time");
		
	label_record_info=gtk_label_new("Storage");
	gtk_label_set_attributes (GTK_LABEL (label_record_info), attrs);
	
	char* record_num_str =" Number of records = ";
	char* n_str = g_strdup_printf("%d", get_total_number_of_events());
	record_num_str = g_strconcat(record_num_str, n_str,NULL);
	label_record_number =gtk_label_new(record_num_str);
	
	char* sqlite_version_str =" Sqlite version  = ";
	char* v_str = g_strdup_printf("%s ", sqlite3_libversion());
	sqlite_version_str = g_strconcat(sqlite_version_str, v_str,NULL);
	label_sqlite_version =gtk_label_new(sqlite_version_str);
	
	
	label_font_info=gtk_label_new("Font");
	gtk_label_set_attributes (GTK_LABEL (label_font_info), attrs);
	
	settings = g_settings_new ("org.gnome.desktop.interface");
	gchar* desktop_font_str = g_settings_get_string (settings, "font-name");
	
	char* desktop_str = "Desktop Font = ";
	desktop_str =g_strconcat(desktop_str, desktop_font_str,NULL);
	label_desktop_font=gtk_label_new(desktop_str);
	
	gdouble sf =g_settings_get_double (settings,"text-scaling-factor");	
	char* gnome_text_scale_factor ="Gnome Text Scale Factor = ";
	char* font_scale_value_str = g_strdup_printf("%0.2lf", sf);
	gnome_text_scale_factor=g_strconcat(gnome_text_scale_factor, font_scale_value_str,NULL);
	label_gnome_text_scale=gtk_label_new(gnome_text_scale_factor);
	
	
	gtk_box_append(GTK_BOX(box),label_keyboard_shortcuts);
	gtk_box_append(GTK_BOX(box),label_home_shortcut);
	gtk_box_append(GTK_BOX(box),label_newevent_shortcut);
	gtk_box_append(GTK_BOX(box),label_editevent_shortcut);
	gtk_box_append(GTK_BOX(box),label_deleteevent_shortcut);
	gtk_box_append(GTK_BOX(box),label_preferences_shortcut);
	gtk_box_append(GTK_BOX(box), label_speak_shortcut);	
	gtk_box_append(GTK_BOX(box), label_time_shortcut);
		
	gtk_box_append(GTK_BOX(box), label_record_info);
	gtk_box_append(GTK_BOX(box), label_record_number);
	gtk_box_append(GTK_BOX(box), label_sqlite_version);
	
	gtk_box_append(GTK_BOX(box),label_font_info);
	gtk_box_append(GTK_BOX(box),label_desktop_font);
	gtk_box_append(GTK_BOX(box),label_gnome_text_scale);
		
	pango_attr_list_unref(attrs);
	gtk_window_present (GTK_WINDOW (dialog));	
}
//======================================================================
static void callbk_about(GSimpleAction* action, GVariant *parameter, gpointer user_data)
{
	GtkWidget *window = user_data;
	const gchar *authors[] = {"Alan Crispin", NULL};
	GtkWidget *about_dialog;
	about_dialog = gtk_about_dialog_new();
	gtk_window_set_transient_for(GTK_WINDOW(about_dialog),GTK_WINDOW(window));
	gtk_widget_set_size_request(about_dialog, 200,200);
    gtk_window_set_modal(GTK_WINDOW(about_dialog),TRUE);
	gtk_about_dialog_set_program_name(GTK_ABOUT_DIALOG(about_dialog), "Talk Calendar");
	gtk_about_dialog_set_version (GTK_ABOUT_DIALOG(about_dialog), "0.1.5");
	gtk_about_dialog_set_copyright(GTK_ABOUT_DIALOG(about_dialog),"Copyright © 2024");
	gtk_about_dialog_set_comments(GTK_ABOUT_DIALOG(about_dialog),"Personal calendar");
	gtk_about_dialog_set_license_type (GTK_ABOUT_DIALOG(about_dialog), GTK_LICENSE_LGPL_2_1);
	gtk_about_dialog_set_website(GTK_ABOUT_DIALOG(about_dialog),"https://github.com/crispinprojects/");
	gtk_about_dialog_set_website_label(GTK_ABOUT_DIALOG(about_dialog),"Talk Calendar Website");
	gtk_about_dialog_set_authors(GTK_ABOUT_DIALOG(about_dialog), authors);
	gtk_about_dialog_set_logo_icon_name(GTK_ABOUT_DIALOG(about_dialog), "x-office-calendar");
	gtk_widget_set_visible (about_dialog, TRUE);	
}
//======================================================================-

//======================================================================
// LISTBOX functions and callbks
//======================================================================

static GtkWidget *create_widget (gpointer item, gpointer user_data)
{
  DisplayItem *obj = (DisplayItem *)item;
  GtkWidget *label;
  label = gtk_label_new (""); 
  
  gtk_widget_set_valign(GTK_WIDGET (label),GTK_ALIGN_BASELINE);
  
  gtk_widget_set_halign (GTK_WIDGET (label), GTK_ALIGN_START); //left align 
  g_object_bind_property (obj, "label", label, "label", G_BINDING_SYNC_CREATE);
  return label;
}
//======================================================================
static void add_separator (GtkListBoxRow *row, GtkListBoxRow *before, gpointer data)
{	
  if (!before) return;
  gtk_list_box_row_set_header (row, gtk_separator_new (GTK_ORIENTATION_HORIZONTAL));
}
//======================================================================
static void callbk_row_activated (GtkListBox *listbox,GtkListBoxRow *row, gpointer user_data)
{
	m_row_index = gtk_list_box_row_get_index (row);	
	DisplayItem *obj = g_list_model_get_item (G_LIST_MODEL (m_store), m_row_index);
	if(obj==NULL) return;
	gint id_value;
	gchar* label_value;
	g_object_get (obj, "id", &id_value, NULL);
	g_object_get (obj, "label", &label_value, NULL);
	m_id_selection=id_value;	
	gint evt_id=0;
	gchar *summary_str="";
	gchar *location_str="";			
	selected_evt = g_object_new(CALENDAR_TYPE_EVENT, 0);
	db_get_event(m_id_selection, selected_evt);
	
	g_object_get (selected_evt, "eventid", &evt_id, NULL);
	g_object_get (selected_evt, "summary", &summary_str, NULL);
	g_object_get (selected_evt, "location", &location_str, NULL);	
		
}
//======================================================================
// Display events (time order)
//======================================================================

static void display_event_array(GArray *evt_arry) {
	
	gint evt_id;
	const gchar *summary_str;
	const gchar *location_str;
	const gchar *description_str;
	gint start_year;
	gint start_month;
	gint start_day;
	gint start_hour;
	gint start_min;
	gint end_year;
	gint end_month;
	gint end_day;
	gint end_hour;
	gint end_min;
	gint is_yearly;
	gint is_allday;
	gint is_priority;
	gint has_notification;
	gint has_reminder;
	gint reminder_min;
	
	// Display
	g_list_store_remove_all(m_store); // clear

	// loop through the events array adding events to listbox store
	for (int i = 0; i < evt_arry->len; i++)	{
		CalendarEvent *evt = g_array_index(evt_arry, CalendarEvent *, i);
		g_object_get(evt, "eventid", &evt_id, NULL);
		g_object_get(evt, "summary", &summary_str, NULL);
		g_object_get(evt, "location", &location_str, NULL);
		g_object_get(evt, "description", &description_str, NULL);
		g_object_get(evt, "startyear", &start_year, NULL);
		g_object_get(evt, "startmonth", &start_month, NULL);
		g_object_get(evt, "startday", &start_day, NULL);
		g_object_get(evt, "starthour", &start_hour, NULL);
		g_object_get(evt, "startmin", &start_min, NULL);
		g_object_get(evt, "endyear", &end_year, NULL);
		g_object_get(evt, "endmonth", &end_month, NULL);
		g_object_get(evt, "endday", &end_day, NULL);
		g_object_get(evt, "endhour", &end_hour, NULL);
		g_object_get(evt, "endmin", &end_min, NULL);
		g_object_get(evt, "isyearly", &is_yearly, NULL);
		g_object_get(evt, "isallday", &is_allday, NULL);
		g_object_get(evt, "ispriority", &is_priority, NULL);
		g_object_get(evt, "hasnotification", &has_notification, NULL);
		g_object_get(evt, "hasreminder", &has_reminder, NULL);
		g_object_get(evt, "remindermin", &reminder_min, NULL);      
            
      gchar *time_str = "";
      gchar *starthour_str = "";
      gchar *startmin_str = "";
      gchar *endhour_str = "";
      gchar *endmin_str = "";
      gchar *ampm_str = " ";

			if (m_12hour_format)
			{

				if (start_hour >= 13 && start_hour <= 23)
				{
					int shour = start_hour;
					shour = shour - 12;
					ampm_str = "pm ";
					starthour_str = g_strdup_printf("%d", shour);
				}
				if(start_hour ==12)
				{
					int shour = start_hour;
					ampm_str = "pm ";
					starthour_str = g_strdup_printf("%d", shour);					
				}
				//else
				if(start_hour >=1 && start_hour<12)
				{
					ampm_str = "am ";
					starthour_str = g_strdup_printf("%d", start_hour);
				}
			} // 12
			else
			{
				starthour_str = g_strdup_printf("%d", start_hour);
			} // 24

			startmin_str = g_strdup_printf("%d", start_min);

			if (start_min < 10)
			{
				time_str = g_strconcat(time_str, starthour_str, ":0", startmin_str, NULL);
			}
			else
			{
				time_str = g_strconcat(time_str, starthour_str, ":", startmin_str, NULL);
			}

			time_str = g_strconcat(time_str, ampm_str, NULL);

			if (m_show_end_time)
			{

				if (m_12hour_format)
				{
					ampm_str = "";

					if (end_hour >= 13 && end_hour <= 23)
					{
						end_hour = end_hour - 12;
						ampm_str = "pm ";
						endhour_str = g_strdup_printf("%d", end_hour);
					}
					if(start_hour ==12)
					{
					int shour = start_hour;
					ampm_str = "pm ";
					starthour_str = g_strdup_printf("%d", end_hour);					
					}
					else
					{
						ampm_str = "am ";
						endhour_str = g_strdup_printf("%d", end_hour);
					}
				} // 12
				else
				{
					endhour_str = g_strdup_printf("%d", end_hour);
				} // 24

				endmin_str = g_strdup_printf("%d", end_min);

				if (end_min < 10)
				{
					time_str = g_strconcat(time_str, "to ", endhour_str, ":0", endmin_str, NULL);
				}
				else
				{
					time_str = g_strconcat(time_str, "to ", endhour_str, ":", endmin_str, NULL);
				}
				time_str = g_strconcat(time_str, ampm_str, NULL);
			} // show_end_time

			if (is_allday)
			{
				time_str = "All day. ";
			}
			else
			{
				time_str = g_strconcat(time_str, NULL);
			}
			
			const gchar *display_str = summary_str; //speech event summary on first line
			
			//display_str = g_strconcat(display_str, "\n", time_str, description_str,". ",location_str, ".", NULL);
			
			if (m_show_location)
			{
				//display location if exists
				if (strlen(location_str) == 0)
				{
					//db sorted
					display_str = g_strconcat(display_str, "\n", time_str, description_str,". ", NULL);
				}
				else
				{
					//display_str = g_strconcat(display_str, time_str, summary_str, ". ",location_str, ".", NULL);
					display_str = g_strconcat(display_str, "\n", time_str, description_str,". ",location_str, ".", NULL);
				}
			}
			else
			{
				//do not display location
				display_str = g_strconcat(display_str, "\n", time_str, description_str,". ", NULL);
			}



			if (is_priority)
			{
				//tag on high priority
				display_str = g_strconcat(display_str, " High Priority.", NULL);
			}
			

			DisplayItem *item = NULL;
			item = g_object_new(display_item_get_type(),
								"id", evt_id,
								"label", display_str,								
								NULL);			
			
			g_list_store_append (m_store, item); //db sorted
			g_object_unref(item);		
		}//evt_arry loop
}

//======================================================================

static GMenu *create_menu(const GtkApplication *app) {
	
		
	GMenu *menu;
    GMenu *file_menu;   
    GMenu *event_menu;
    GMenu *calendar_menu;
    GMenu *help_menu;
    GMenuItem *item;

	menu =g_menu_new();
	file_menu =g_menu_new();	
	event_menu =g_menu_new();
	calendar_menu =g_menu_new();
	help_menu =g_menu_new();
	
	//File items	
	item =g_menu_item_new("Export", "app.export");
	g_menu_append_item(file_menu,item);
	g_object_unref(item);
	
	item =g_menu_item_new("Import", "app.import");
	g_menu_append_item(file_menu,item);
	g_object_unref(item);
	
	item =g_menu_item_new("Quit", "app.quit");
	g_menu_append_item(file_menu,item);
	g_object_unref(item);
		
		
	//Event items
	item =g_menu_item_new("New Event", "app.newevent");
	g_menu_append_item(event_menu,item);
	g_object_unref(item);
	
	item =g_menu_item_new("Edit Selected Event", "app.editevent");
	g_menu_append_item(event_menu,item);
	g_object_unref(item);
	
	item =g_menu_item_new("Delete Selected Event", "app.deleteevent");
	g_menu_append_item(event_menu,item);
	g_object_unref(item);
	
	item =g_menu_item_new("Delete All Events", "app.deleteall");
	g_menu_append_item(event_menu,item);
	g_object_unref(item);
	
	item =g_menu_item_new("Speak", "app.speak");
	g_menu_append_item(event_menu,item);
	g_object_unref(item);
	
	//Calendar items
	item =g_menu_item_new("Go To Today", "app.home");
	g_menu_append_item(calendar_menu,item);
	g_object_unref(item);
	
	item =g_menu_item_new("Speak Time", "app.speaktime");
	g_menu_append_item(calendar_menu,item);
	g_object_unref(item);
		
	item =g_menu_item_new("Colour", "app.colour");
	g_menu_append_item(calendar_menu,item);
	g_object_unref(item);
	
	item =g_menu_item_new("Preferences", "app.preferences");
	g_menu_append_item(calendar_menu,item);
	g_object_unref(item);	
		
	//Help items
	item =g_menu_item_new("Information", "app.info");
	g_menu_append_item(help_menu,item);
	g_object_unref(item);
	
	item =g_menu_item_new("About", "app.about");
	g_menu_append_item(help_menu,item);
	g_object_unref(item);
	
	g_menu_append_submenu(menu, "File", G_MENU_MODEL(file_menu));
    g_object_unref(file_menu);   
    g_menu_append_submenu(menu, "Event", G_MENU_MODEL(event_menu));
    g_object_unref(event_menu);
    g_menu_append_submenu(menu, "Calendar", G_MENU_MODEL(calendar_menu));
    g_object_unref(calendar_menu);
    g_menu_append_submenu(menu, "Help", G_MENU_MODEL(help_menu));
    g_object_unref(help_menu);
    
    return menu;
}
//=====================================================================
//thread playraw
//======================================================================

static gpointer thread_playraw(gpointer user_data)
{  
    gchar *raw_file =user_data;       
    gchar *m_sample_rate_str = g_strdup_printf("%i", m_talk_rate); 
    gchar *sample_rate_str ="-r ";    
    sample_rate_str= g_strconcat(sample_rate_str,m_sample_rate_str, NULL);     
    gchar * command_str ="aplay -c 1 -f S16_LE";
    //gchar * command_str ="aplay -c 1 -f U8";
     //gchar * command_str ="aplay -c 1 -f U8";
    command_str =g_strconcat(command_str," ",sample_rate_str, " ", raw_file, NULL); 
    system(command_str); 
           
    g_mutex_unlock (&lock); //thread mutex unlock 
    return NULL; 
}
//======================================================================

unsigned char *rawcat(unsigned char *arrys[], unsigned int arry_size[], int arry_count) {
		
	
	if (arry_count<2) return NULL;	
	
	unsigned int  total_samples=0;
	for (int c = 0; c < arry_count; c++) 
	{  
    unsigned int count =arry_size[c]; 
    total_samples=total_samples+count;	
    }        
	unsigned char *data = (unsigned char*)malloc(total_samples * sizeof(unsigned char));
	
	unsigned int offset=0;
	for(int k=0; k<arry_count; k++)
	{
		//loop through each arry	
		for(int i = 0; i < arry_size[k]; i++) 
		{		
		data[i+offset]=arrys[k][i];
		}		
		offset =offset+arry_size[k];
	}//k kount
	return data;
}
//======================================================================
unsigned int get_merge_size(unsigned int sizes_arry[], int arry_size){
	
	unsigned int total_samples=0;
	for (int i = 0; i < arry_size; i++) 
	{  
    unsigned int count =sizes_arry[i]; 
    total_samples=total_samples+count;	
    }
	return total_samples;
}
//======================================================================
static void callbk_speak(GSimpleAction* action, GVariant *parameter,gpointer user_data){
		
	speak_events();
}
//======================================================================

static char* get_day_of_week(int day, int month, int year) 
{

	char* weekday_str="";
	GDate* day_date;
	day_date = g_date_new_dmy(day, month, year);
	GDateWeekday weekday =g_date_get_weekday(day_date);
		
	switch(weekday)
	{
	case G_DATE_MONDAY:
	weekday_str="monday";;
	break;
	case G_DATE_TUESDAY:
	weekday_str="tuesday";;
	break;
	case G_DATE_WEDNESDAY:
	weekday_str="wednesday";
	break;
	case G_DATE_THURSDAY:
	weekday_str="thursday";
	break;
	case G_DATE_FRIDAY:
	weekday_str="friday";
	break;
	case G_DATE_SATURDAY:
	weekday_str="saturday";
	break;
	case G_DATE_SUNDAY:
	weekday_str="sunday";
	break;
	default:
	weekday_str="unknown";
	}//switch

	return weekday_str;
}
//======================================================================
static char* get_day_number_ordinal_string(int day) 
{

	char* day_str ="";

	switch (day) {
		case 1:
		day_str="first";
		break;
		case 2:
		day_str="second";
		break;
		case 3:
		day_str="third";
		break;
		case 4:
		day_str="fourth";
		break;
		case 5:
		day_str="fifth";
		break;
		case 6:
		day_str="sixth";
		break;
		case 7:
		day_str="seventh";
		break;
		case 8:
		day_str="eighth";
		break;
		case 9:
		day_str="ninth";
		break;
		case 10:
		day_str="tenth";
		break;
		case 11:
		day_str="eleventh";
		break;
		case 12:
		day_str="twelfth";
		break;
		case 13:
		day_str="thirteenth";
		break;
		case 14:
		day_str="fourteenth";
		break;
		case 15:
		day_str="fifteenth";

		break;
		case 16:
		day_str="sixteenth";
		break;
		case 17:
		day_str="seventeenth";
		break;
		case 18:
		day_str="eighteenth";
		break;
		case 19:
		day_str="nineteenth";
		break;
		case 20:
		day_str="twentieth";
		break;
		case 21:
		day_str="twentyfirst";
		break;
		case 22:
		day_str="twentysecond";
		break;
		case 23:
		day_str="twentythird";
		break;
		case 24:
		day_str="twentyfourth";
		break;
		case 25:
		day_str="twentyfifth";
		break;
		case 26:
		day_str="twentysixth";
		break;
		case 27:
		day_str="twentyseventh";
		break;
		case 28:
		day_str="twentyeighth";
		break;
		case 29:
		day_str="twentynineth";
		break;
		case 30:
		day_str="thirtieth";
		break;
		case 31:
		day_str="thirtyfirst";
		break;
		default:
		//Unknown day ordinal
		day_str="unknown";
		break;
	  } //day switch
	return day_str;
}
//======================================================================
char* get_month_string(int month) {

	char* result ="";
	
	switch(month) {
	case 1:
		result = "january";
		break;
	case 2:
		result = "february";
		break;
	case 3:
		result= "march";
		break;
	case 4:
		result = "april";
		break;
	case 5:
		result ="may";
		break;
	case 6:
		result = "june";
		break;
	case 7:
		result ="july";
		break;
	case 8:
		result ="august";
		break;
	case 9:
		result= "september";
		break;
	case 10:
		result = "october";
		break;
	case 11:
		result = "november";
		break;
	case 12:
		result = "december";
		break;
	default:
		result = "unknown";
	}
	return result;
}
//======================================================================

static char* get_event_title_word(char* word) 
{

char* result="";

	if (g_strcmp0(word,"activity")==0) {
	result ="activity";
	}	
	if (g_strcmp0(word,"anniversary")==0) {
	result="anniversary";
	}
	if (g_strcmp0(word,"appointment")==0) {
	result="appointment";
	}	
	if (g_strcmp0(word,"birthday")==0) {
	result="birthday";
	}	
	if (g_strcmp0(word,"cafe")==0) {
	result="cafe";
	}
	if (g_strcmp0(word,"car")==0) {
	result="car";
	}
	//dwords
	if (g_strcmp0(word,"dentist")==0) {
	result="dentist";
	}
	if (g_strcmp0(word,"doctor")==0) {
	result="doctor";
	}
	if (g_strcmp0(word,"driver")==0) {
	result="driver";
	}
	//f words
	if (g_strcmp0(word,"family")==0) {
	result ="family";
	}
	if (g_strcmp0(word,"funeral")==0) {
	result ="funeral";
	}
	if (g_strcmp0(word,"garden")==0) {
	result="garden";
	}		
	if (g_strcmp0(word,"holiday")==0) {
	result="holiday";
	}
	if (g_strcmp0(word,"hospital")==0) {
	result="hospital";
	}
	
	if (g_strcmp0(word,"meeting")==0) {
	result="meeting";
	}
	if (g_strcmp0(word,"meetup")==0) {
	result="meetup";
	}
	//pwords
	if (g_strcmp0(word,"party")==0) {
	result="party";
	}
	if (g_strcmp0(word,"payment")==0) {
	result="payment";
	}
	//rwords
	if (g_strcmp0(word,"reminder")==0) {
	result="reminder";
	}
	if (g_strcmp0(word,"restaurant")==0) {
	result="restaurant";
	}
	// s words
	// t words
	if (g_strcmp0(word,"task")==0) {
	result="task";
	}
	if (g_strcmp0(word,"travel")==0) {
	result="travel";
	}
	//vwords
	if (g_strcmp0(word,"visit")==0) {
	result="visit";
	}		
	if (g_strcmp0(word,"work")==0) {
	result="work";
	}

return result;
}
//======================================================================
static char* get_cardinal_string(int number)
{

	char* result ="zero";

     switch(number)
     {
         //case 0:
		 //result ="zero";
         case 1:
		 result ="one";
		 break;
		 case 2:
		 result ="two";
		 break;
		 case 3:
		 result = "three";
		 break;
		 case 4:
		 result ="four";
		 break;
		 case 5:
		 result ="five";
		 break;
		 case 6:
		 result ="six";
		 break;
		 case 7:
		 result ="seven";
		 break;
		 case 8:
		 result="eight";
		 break;
		 case 9:
		 result="nine";
		 break;
		 case 10:
		 result="ten";
		 break;
		 case 11:
		 result="eleven";
		 break;
		 case 12:
		 result="twelve";
		 break;
		 case 13:
		 result="thirteen";
		 break;
		 case 14:
		 result ="fourteen";
		 break;
		 case 15:
		 result ="fifteen";
		 break;
		 case 16:
		 result="sixteen";
		 break;
		 case 17:
		 result="seventeen";
		 break;
		 case 18:
		 result="eighteen";
		 break;
		 case 19:
		 result="nineteen";
		 break;
		 case 20:
		 result ="twenty";
		 break;
		 case 21:
		 result="twentyone";
		 break;
		 case 22:
		 result="twentytwo";
		 break;
		 case 23:
		 result="twentythree";
		 break;
		 case 24:
		 result="twentyfour";
		 break;
		 case 25:
		 result="twentyfive";
		 break;
		 case 26:
		 result="twentysix";
		 break;
		 case 27:
		 result="twentyseven";
		 break;
		 case 28:
		 result="twentyeight";
		 break;
		 case 29:
		 result="twentynine";
		 break;
		 case 30:
		 result="thirty";
		 break;
		 case 31:
		 result="thirtyone";
		 break;
		 case 32:
		 result="thirtytwo";
		 break;
		 case 33:
		 result="thirtythree";
		 break;
		 case 34:
		 result="thirtyfour";
		 break;
		 case 35:
		 result="thirtyfive";
		 break;
		 case 36:
		 result="thirtysix";
		 break;
		 case 37:
		 result="thirtyseven";
		 break;
		 case 38:
		 result="thirtyeight";
		 break;
		 case 39:
		 result="thirtynine";
		 break;
		 case 40:
		 result="forty";
		 break;
		 case 41:
		 result="fortyone";
		 break;
		 case 42:
		 result="fortytwo";
		 break;
		 case 43:
		 result="fortythree";
		 break;
		 case 44:
		 result="fortyfour";
		 break;
		 case 45:
		 result="fortyfive";
		 break;
		 case 46:
		 result="fortysix";
		 break;
		 case 47:
		 result="fortyseven";
		 break;
		 case 48:
		 result="fortyeight";
		 break;
		 case 49:
		 result="fortynine";
		 break;
		 case 50:
		 result="fifty";
		 break;
		 case 51:
		 result="fiftyone";
		 break;
		 case 52:
		 result="fiftytwo";
		 break;
		 case 53:
		 result="fiftythree";
		 break;
		 case 54:
		 result="fiftyfour";
		 break;
		 case 55:
		 result="fiftyfive";
		 break;
		 case 56:
		 result="fiftysix";
		 break;
		 case 57:
		 result="fiftyseven";
		 break;
		 case 58:
		 result="fiftyeight";
		 break;
		 case 59:
		 result="fiftynine";
		 break;  
         default:
           g_print ("default: number is: %i\n", number);
	 }//switch start hour
	return result;

}
//======================================================================
GArray*  get_upcoming_array(int upcoming_days) 
{
	if(upcoming_days > 7) upcoming_days=7; //clamp
	
	int day1=1; //start day
	int day2=1; //end day
		
	GDate *today_date;
	today_date = g_date_new();
	g_date_set_time_t(today_date, time(NULL));
	int today = g_date_get_day(today_date);
	int month = g_date_get_month(today_date);
	int year = g_date_get_year(today_date);
	g_date_free(today_date); // freeit quick
	
	guint8 month_days =g_date_get_days_in_month(month,year);	
		
	if(((today+1)+upcoming_days)<=month_days)
	{
		//Normal case
		GArray *evt_arry_upcoming;
	    evt_arry_upcoming = g_array_new(FALSE, FALSE, sizeof(CALENDAR_TYPE_EVENT)); 	
		
		day1=today+1;
		day2=today+upcoming_days;
		db_get_upcoming_events(evt_arry_upcoming,year,month,day1,day2);	
	    return evt_arry_upcoming;		
	}
	
	else {
		//special edge cases 
		
		if (today == month_days){
		//Special case: today is last day of month 
		//so today+1 will be first day of next month
		
		GArray *evt_arry_upcoming;
		evt_arry_upcoming = g_array_new(FALSE, FALSE, sizeof(CALENDAR_TYPE_EVENT)); 
			
		day1=1;
		day2=upcoming_days;
		month=month+1; //next month
		
		if(month>12) { //december then becomes january
			month =1; 	
			year=year+1;
		}
		
		db_get_upcoming_events(evt_arry_upcoming,year,month,day1,day2);	
		return evt_arry_upcoming;	
		}
		else
		{	
		//complex two month case
		int day1=today+1;
		
		int day_diff=(today+upcoming_days)-month_days;		
		
		GArray *total_arry;
	    total_arry= g_array_new(FALSE, FALSE, sizeof(CALENDAR_TYPE_EVENT)); 
	    
	    GArray *month1_arry;
	    month1_arry= g_array_new(FALSE, FALSE, sizeof(CALENDAR_TYPE_EVENT));
	    db_get_upcoming_events(month1_arry,year,month,day1,month_days); 
	    
	    //return month1_arry;
	    //Append month1_arry to total_arry
	    for (int i = 0; i < month1_arry->len; i++)
	    {
		CalendarEvent *evt = g_array_index(month1_arry, CalendarEvent *, i);
		g_array_append_val(total_arry, evt);		
		}
	   
	    int next_month=1;
	    //find next month and year
	    if(month+1>12) {
	    next_month=1;
	    year=year+1;
	    } 
	    else {
			next_month=month+1;
		}
	     
	    GArray *month2_arry;
	    month2_arry= g_array_new(FALSE, FALSE, sizeof(CALENDAR_TYPE_EVENT)); 
	    db_get_upcoming_events(month2_arry,year,next_month,1,day_diff); 
	    
	    //Append  month2_arry to total arry	    
	    for (int j = 0; j < month2_arry->len; j++)
	    {
		CalendarEvent *evt = g_array_index(month2_arry, CalendarEvent *, j);
		g_array_append_val(total_arry, evt);		
		}	
		return total_arry;	
		
		} //else 2 month edge case
		
	}//else special cases
	
return NULL; //should never get here but if it does return NULL
}
//======================================================================

static void speak_events() {
	
	if(m_talk==0) return;
	
	GList *speak_word_list = NULL;
	
	//---------------------------------------------------------------
	
	gchar *dow_str=get_day_of_week(m_start_day, m_start_month, m_start_year);	//get day of week
	gchar *day_number_str=get_day_number_ordinal_string(m_start_day); //get day number
	gchar *month_str=get_month_string(m_start_month); //get month
	
	speak_word_list = g_list_append(speak_word_list, dow_str);
	speak_word_list = g_list_append(speak_word_list, day_number_str);
	speak_word_list = g_list_append(speak_word_list, month_str);
		
	
	//Check for public holidays and special days (Christmas, Easter, Fathers etc,.)
	//---------------------------------------------------------------
	if ((m_holidays ==1) && (is_notable_date(m_start_day)))		
	//if (m_holidays ==1)
	{
		//markup public holidays
	if (m_start_month==1 && m_start_day ==1) {
	speak_word_list = g_list_append(speak_word_list, "new");
	speak_word_list = g_list_append(speak_word_list, "year");
	}

	if (m_start_month==12 && m_start_day==25) {
	//christmas day
	speak_word_list = g_list_append(speak_word_list, "christmas");
	speak_word_list = g_list_append(speak_word_list, "day");
	}

	if (m_start_month==12 && m_start_day==26) {
	//boxing day
	speak_word_list = g_list_append(speak_word_list, "boxing");
	speak_word_list = g_list_append(speak_word_list, "day");
	}
	
	if (m_start_month==2 && m_start_day==14) {
	//valentine day
	speak_word_list = g_list_append(speak_word_list, "valentine");
	speak_word_list = g_list_append(speak_word_list, "day");	
	}
		
	//Easter		
	GDate *easter_date =calculate_easter(m_start_year);
	int easter_day = g_date_get_day(easter_date);
	int easter_month =g_date_get_month(easter_date);

	if(m_start_month==easter_month && m_start_day == easter_day)
	{
	//easter day
	speak_word_list = g_list_append(speak_word_list, "easter");
	speak_word_list = g_list_append(speak_word_list, "day");
	}
	
	g_date_subtract_days(easter_date,2);
	int easter_friday = g_date_get_day(easter_date);
	int easter_friday_month =g_date_get_month(easter_date);

	if(m_start_month==easter_friday_month && m_start_day ==easter_friday)
	{
	//easter friday
	speak_word_list = g_list_append(speak_word_list, "easter");
	speak_word_list = g_list_append(speak_word_list, "friday");
	}
	g_date_add_days(easter_date,3);
	int easter_monday = g_date_get_day(easter_date); //easter monday
	int easter_monday_month =g_date_get_month(easter_date);

	if(m_start_month==easter_monday_month && m_start_day ==easter_monday)
	{
	//easter monday
	speak_word_list = g_list_append(speak_word_list, "easter");
	speak_word_list = g_list_append(speak_word_list, "monday");
	}
	
	//BST starts
	GDate *bst_start_date =calculate_bst_start();
	int bst_start_day = g_date_get_day(bst_start_date);
	int bst_start_month =g_date_get_month(bst_start_date);
	
	if(m_start_month==bst_start_month && m_start_day == bst_start_day)
	{
	//bst starts
	//date_str= "BST Starts Clock Forward";
	//date_str =g_strconcat(date_str," BST Starts Clocks Forward ", NULL);
	speak_word_list = g_list_append(speak_word_list, "bst");
	speak_word_list = g_list_append(speak_word_list, "begins");
	speak_word_list = g_list_append(speak_word_list, "clocks");
	speak_word_list = g_list_append(speak_word_list, "forward");
	speak_word_list = g_list_append(speak_word_list, "one");
	speak_word_list = g_list_append(speak_word_list, "hour");
	
	}
	
	
	//May complicated
	if (m_start_month == 5) {
     GDate *first_monday_may;
     first_monday_may = g_date_new_dmy(1, m_start_month, m_start_year);

     while (g_date_get_weekday(first_monday_may) != G_DATE_MONDAY)
       g_date_add_days(first_monday_may,1);

     int may_day = g_date_get_day(first_monday_may);

     if( m_start_day==may_day) {		
		 speak_word_list = g_list_append(speak_word_list, "bank");
	     speak_word_list = g_list_append(speak_word_list, "holiday");
	 }
      
     int days_in_may =g_date_get_days_in_month (m_start_month, m_start_year);

     int plus_days = 0;

     if (may_day + 28 <= days_in_may) {
       plus_days = 28;
     } else {
       plus_days = 21;
     }

     GDate *spring_bank =g_date_new_dmy (may_day, m_start_month, m_start_year);
     g_date_add_days(spring_bank,plus_days);
     int spring_bank_day = g_date_get_day(spring_bank);
     if (g_date_valid_dmy (spring_bank_day,m_start_month,m_start_year) && m_start_day ==spring_bank_day)
     {
	 speak_word_list = g_list_append(speak_word_list, "spring");
     speak_word_list = g_list_append(speak_word_list, "bank");
     speak_word_list = g_list_append(speak_word_list, "holiday");     
     }
	} //m_start_month ==5 (May)
	
	//August complicated
	if (m_start_month == 8) {      
     GDate *first_monday_august;
     first_monday_august = g_date_new_dmy(1, m_start_month, m_start_year);

     while (g_date_get_weekday(first_monday_august) != G_DATE_MONDAY)
       g_date_add_days(first_monday_august,1);

     int august_day = g_date_get_day(first_monday_august);

     int days_in_august =g_date_get_days_in_month (m_start_month, m_start_year);
     int plus_days = 0;

     if (august_day + 28 <= days_in_august) {
       plus_days = 28;
     } else {
       plus_days = 21;
     }

     GDate *august_bank =g_date_new_dmy (august_day, m_start_month, m_start_year);

     g_date_add_days(august_bank,plus_days);

     int august_bank_day = g_date_get_day(august_bank);

	if (g_date_valid_dmy (august_bank_day,m_start_month,m_start_year) && m_start_day ==august_bank_day)
	{
	//return " August Bank Holiday ";   //august bank holiday	
	speak_word_list = g_list_append(speak_word_list, "bank");
	speak_word_list = g_list_append(speak_word_list, "holiday"); 
	}

    } //m_start_month==8 (august)
    
    
    GDate *bst_end_date =calculate_bst_end();
    int bst_end_day = g_date_get_day(bst_end_date);
	int bst_end_month =g_date_get_month(bst_end_date);
	
	if(m_start_month==bst_end_month && m_start_day == bst_end_day)
	{
	//bst ends
	//date_str= "BST Ends Clocks Back";
	speak_word_list = g_list_append(speak_word_list, "bst");
	speak_word_list = g_list_append(speak_word_list, "ends");
	speak_word_list = g_list_append(speak_word_list, "clocks");
	speak_word_list = g_list_append(speak_word_list, "back");
	speak_word_list = g_list_append(speak_word_list, "one");
	speak_word_list = g_list_append(speak_word_list, "hour");
	}
		
	} //if public holidays 
	

	
	//cycle through day events adding event titles
	GArray *day_events_arry =g_array_new(FALSE, FALSE, sizeof(CALENDAR_TYPE_EVENT));
	db_get_all_events_year_month_day(day_events_arry , m_start_year,m_start_month, m_start_day);
	int event_count = day_events_arry->len;
	
	//Talk event number if required
	
	if(m_talk_event_number) {
	int event_number = day_events_arry->len;		
	if (event_number ==0) {			
	speak_word_list = g_list_append(speak_word_list, "no");
	speak_word_list = g_list_append(speak_word_list, "events");
	//speak_word_list = g_list_append(speak_word_list, "today");		
	} //if count=0		
	else if(event_number ==1){		
	speak_word_list = g_list_append(speak_word_list, "one");
	speak_word_list = g_list_append(speak_word_list, "event");
	}
	else if(event_number ==2){		
	speak_word_list = g_list_append(speak_word_list, "two");
	speak_word_list = g_list_append(speak_word_list, "events");
	}
	else if(event_number ==3){
	speak_word_list = g_list_append(speak_word_list, "three");
	speak_word_list = g_list_append(speak_word_list, "events"); 
	}
	else if(event_number ==4){
	speak_word_list = g_list_append(speak_word_list, "four");
	speak_word_list = g_list_append(speak_word_list, "events");
	}
	else if(event_number ==5){ 
	speak_word_list = g_list_append(speak_word_list, "five");
	speak_word_list = g_list_append(speak_word_list, "events");		
	}		
	else {
	speak_word_list = g_list_append(speak_word_list, "many");
	speak_word_list = g_list_append(speak_word_list, "events");
	}	    	
	} //m_talk_event_number
	
	//Talk time and event word if required	
	for (int i = 0; i < day_events_arry->len; i++)
	{
		gint evt_id = 0;
		gchar *summary_str = "";
		gchar *event_number_str="";
		gint start_hour = 0;
		gint start_min = 0;
		gint is_allday = 0;
		//gint is_priority = 0;
		
		CalendarEvent *evt = g_array_index(day_events_arry, CalendarEvent *, i);
		
		g_object_get(evt, "summary", &summary_str, NULL);		
		g_object_get(evt, "starthour", &start_hour, NULL);
		g_object_get(evt, "startmin", &start_min, NULL);		
		g_object_get(evt, "isallday", &is_allday, NULL);		
		
		//Get time first
		gchar* hour_str="";
		gchar* min_str="";
		gchar* ampm_str="";
		
		if(m_talk_time) {
		
		//if(is_allday)
		//{
		//// no time
		////speak_word_list = g_list_append(speak_word_list, "all");
		////speak_word_list = g_list_append(speak_word_list, "day");
		////speak_word_list = g_list_append(speak_word_list, "event");
		//}		
		if(!is_allday) {		
		if(m_12hour_format) {
		
		if(start_hour >=1 && start_hour<12) {
		//AM
		hour_str =get_cardinal_string(start_hour);
		
		ampm_str="am";
		speak_word_list = g_list_append(speak_word_list, hour_str);
		
		if(start_min!=0){
		min_str=get_cardinal_string(start_min);				    
		}
		speak_word_list = g_list_append(speak_word_list, min_str);
		speak_word_list = g_list_append(speak_word_list, ampm_str);				
		}
		else if(start_hour ==12) {
		//PM
		hour_str =get_cardinal_string(start_hour);
		min_str=get_cardinal_string(start_min);
		ampm_str="pm";
		speak_word_list = g_list_append(speak_word_list, hour_str);
		
		if(start_min!=0){
		min_str=get_cardinal_string(start_min);
		speak_word_list = g_list_append(speak_word_list, min_str);
		}				
		speak_word_list = g_list_append(speak_word_list, ampm_str);
		}
		else if (start_hour >=13 && start_hour<=23) {
		//PM
		start_hour=start_hour-12;
		hour_str =get_cardinal_string(start_hour);
		min_str=get_cardinal_string(start_min);
		ampm_str="pm";
		speak_word_list = g_list_append(speak_word_list, hour_str);
		if(start_min!=0){
		min_str=get_cardinal_string(start_min);
		speak_word_list = g_list_append(speak_word_list, min_str);
		}
		speak_word_list = g_list_append(speak_word_list, ampm_str);				
		}	
		
		} //12hour format
		else
		{				
		hour_str =get_cardinal_string(start_hour);
		min_str=get_cardinal_string(start_min);
		speak_word_list = g_list_append(speak_word_list, hour_str);
		speak_word_list = g_list_append(speak_word_list, min_str);			    				
		} //24 hour format	
		} // not all_day		
		}//m_talk_time
		
		//now add event title
		if(m_talk_event_words) {
		gchar* summary_lower= g_ascii_strdown(summary_str,-1);		
		gchar *event_title =get_event_title_word(summary_lower);		
		speak_word_list = g_list_append(speak_word_list, event_title);
		}//m_talk_event_words	
		
	} //for
	
	    //upcoming
	    GDate *today_date;
		today_date = g_date_new();
		g_date_set_time_t(today_date, time(NULL));
		int today = g_date_get_day(today_date);
		int month = g_date_get_month(today_date);
		int year = g_date_get_year(today_date);
		g_date_free(today_date); // freeit quick
		
		//find upcoming for today
		if(m_talk_upcoming && m_start_day==today && m_start_month ==month && m_start_year==year)  
		{		
			
			GArray *evts_upcoming = get_upcoming_array(7); //next 7 days		
			int num_upcoming = evts_upcoming->len;	
			
			
			if (num_upcoming ==0) {			
			speak_word_list = g_list_append(speak_word_list, "no");
			speak_word_list = g_list_append(speak_word_list, "upcoming");
			speak_word_list = g_list_append(speak_word_list, "events");		
			} //if count=0		
			else if(num_upcoming ==1){		
			speak_word_list = g_list_append(speak_word_list, "one");
			speak_word_list = g_list_append(speak_word_list, "upcoming");
			speak_word_list = g_list_append(speak_word_list, "event");
			}
			else if(num_upcoming ==2){		
			speak_word_list = g_list_append(speak_word_list, "two");
			speak_word_list = g_list_append(speak_word_list, "upcoming");
			speak_word_list = g_list_append(speak_word_list, "events");
			}
			else if(num_upcoming ==3){
			speak_word_list = g_list_append(speak_word_list, "three");
			speak_word_list = g_list_append(speak_word_list, "upcoming");
			speak_word_list = g_list_append(speak_word_list, "events"); 
			}
			else if(num_upcoming ==4){
			speak_word_list = g_list_append(speak_word_list, "four");
			speak_word_list = g_list_append(speak_word_list, "upcoming");
			speak_word_list = g_list_append(speak_word_list, "events");
			}
			else if(num_upcoming ==5){ 
			speak_word_list = g_list_append(speak_word_list, "five");
			speak_word_list = g_list_append(speak_word_list, "upcoming");
			speak_word_list = g_list_append(speak_word_list, "events");		
			}		
			else {
			speak_word_list = g_list_append(speak_word_list, "many");
			speak_word_list = g_list_append(speak_word_list, "upcoming");
			speak_word_list = g_list_append(speak_word_list, "events");
			}	    	
						
			for (int i = 0; i < evts_upcoming->len; i++)
	        {
				//find titles
				gchar *summary_str = "";
				gint is_priority;
				CalendarEvent *evt = g_array_index(evts_upcoming, CalendarEvent *, i);
				g_object_get(evt, "summary", &summary_str, NULL);
				g_object_get(evt, "ispriority", &is_priority, NULL);
				speak_word_list = g_list_append(speak_word_list, summary_str);
				if(is_priority) {
					speak_word_list = g_list_append(speak_word_list, "high");
					speak_word_list = g_list_append(speak_word_list, "priority");
				}				
			}
					
		}//m_talk_upcoming	
	
	//print out word list	
	//cycle through the event type word list
	gpointer word_list_pointer;
	gchar* word_str;
	gchar* word_str_lower;	
	gint word_number  =g_list_length(speak_word_list);
	
	//create word array using list size
	unsigned char *word_arrays[word_number]; 
	unsigned int word_arrays_sizes[word_number];
	//load hex words into array
		
	
	//load word array
	for(int i=0; i < word_number; i++)
	{
		word_list_pointer=g_list_nth_data(speak_word_list,i);
		word_str=(gchar *)word_list_pointer;
		gchar* word_str_lower= g_ascii_strdown(word_str,-1);	//make sure lower			
		//load up arrays
		//dummy
		
		word_arrays[i] = (unsigned char*)malloc(empty_raw_len * sizeof(unsigned char));
		word_arrays[i] = empty_raw;
		word_arrays_sizes[i]=empty_raw_len; 
				
		if (g_strcmp0(word_str_lower,"monday")==0) {		
		word_arrays[i] = (unsigned char*)malloc(monday_raw_len * sizeof(unsigned char));
		word_arrays[i] = monday_raw;
		word_arrays_sizes[i]=monday_raw_len;
	    } 		
		if (g_strcmp0(word_str_lower,"tuesday")==0) {			
		word_arrays[i] = (unsigned char*)malloc(tuesday_raw_len * sizeof(unsigned char));
		word_arrays[i] = tuesday_raw;
		word_arrays_sizes[i]=tuesday_raw_len;
	    }
		if (g_strcmp0(word_str_lower,"wednesday")==0) {		
		word_arrays[i] = (unsigned char*)malloc(wednesday_raw_len * sizeof(unsigned char));
		word_arrays[i] = wednesday_raw;
		word_arrays_sizes[i]=wednesday_raw_len; 
	    }
		if (g_strcmp0(word_str_lower,"thursday")==0) {	
		word_arrays[i] = (unsigned char*)malloc(thursday_raw_len * sizeof(unsigned char));
		word_arrays[i] = thursday_raw;
		word_arrays_sizes[i]=thursday_raw_len; 
	    }
	    if (g_strcmp0(word_str_lower,"friday")==0) {	
		word_arrays[i] = (unsigned char*)malloc(friday_raw_len * sizeof(unsigned char));
		word_arrays[i] = friday_raw;
		word_arrays_sizes[i]=friday_raw_len;  
	    }
		if (g_strcmp0(word_str_lower,"saturday")==0) {		
		word_arrays[i] = (unsigned char*)malloc(saturday_raw_len * sizeof(unsigned char));
		word_arrays[i] = saturday_raw;
		word_arrays_sizes[i]=saturday_raw_len; 
	    }
	    if (g_strcmp0(word_str_lower,"sunday")==0) {		
		word_arrays[i] = (unsigned char*)malloc(sunday_raw_len * sizeof(unsigned char));
		word_arrays[i] = sunday_raw;
		word_arrays_sizes[i]=sunday_raw_len;
	    }
	    
	    if (g_strcmp0(word_str_lower,"first")==0) {		
		word_arrays[i] = (unsigned char*)malloc(first_raw_len * sizeof(unsigned char));
		word_arrays[i] = first_raw;
		word_arrays_sizes[i]=first_raw_len;	
	    }
	    if (g_strcmp0(word_str_lower,"second")==0) {		
		word_arrays[i] = (unsigned char*)malloc(second_raw_len * sizeof(unsigned char));
		word_arrays[i] = second_raw;
		word_arrays_sizes[i]=second_raw_len;	
	    }
	    if (g_strcmp0(word_str_lower,"third")==0) {		
		word_arrays[i] = (unsigned char*)malloc(third_raw_len * sizeof(unsigned char));
		word_arrays[i] = third_raw;
		word_arrays_sizes[i]=third_raw_len;	
	    }
	    if (g_strcmp0(word_str_lower,"fourth")==0) {		
		word_arrays[i] = (unsigned char*)malloc(fourth_raw_len * sizeof(unsigned char));
		word_arrays[i] = fourth_raw;
		word_arrays_sizes[i]=fourth_raw_len;	
	    }
	    if (g_strcmp0(word_str_lower,"fifth")==0) {		
		word_arrays[i] = (unsigned char*)malloc(fifth_raw_len * sizeof(unsigned char));
		word_arrays[i] = fifth_raw;
		word_arrays_sizes[i]=fifth_raw_len;	
	    }
	    if (g_strcmp0(word_str_lower,"sixth")==0) {		
		word_arrays[i] = (unsigned char*)malloc(sixth_raw_len * sizeof(unsigned char));
		word_arrays[i] = sixth_raw;
		word_arrays_sizes[i]=sixth_raw_len;	
	    }
	    if (g_strcmp0(word_str_lower,"seventh")==0) {		
		word_arrays[i] = (unsigned char*)malloc(seventh_raw_len * sizeof(unsigned char));
		word_arrays[i] = seventh_raw;
		word_arrays_sizes[i]=seventh_raw_len;	
	    }
	    if (g_strcmp0(word_str_lower,"eighth")==0) {		
		word_arrays[i] = (unsigned char*)malloc(eighth_raw_len * sizeof(unsigned char));
		word_arrays[i] = eighth_raw;
		word_arrays_sizes[i]=eighth_raw_len;	
	    }
	    if (g_strcmp0(word_str_lower,"ninth")==0) {		
		word_arrays[i] = (unsigned char*)malloc(ninth_raw_len * sizeof(unsigned char));
		word_arrays[i] = ninth_raw;
		word_arrays_sizes[i]=ninth_raw_len;	
	    }
	    if (g_strcmp0(word_str_lower,"tenth")==0) {		
		word_arrays[i] = (unsigned char*)malloc(tenth_raw_len * sizeof(unsigned char));
		word_arrays[i] = tenth_raw;
		word_arrays_sizes[i]=tenth_raw_len;	
	    }
	    if (g_strcmp0(word_str_lower,"eleventh")==0) {		
		word_arrays[i] = (unsigned char*)malloc(eleventh_raw_len * sizeof(unsigned char));
		word_arrays[i] = eleventh_raw;
		word_arrays_sizes[i]=eleventh_raw_len;	
	    }
	    if (g_strcmp0(word_str_lower,"twelfth")==0) {		
		word_arrays[i] = (unsigned char*)malloc(twelfth_raw_len * sizeof(unsigned char));
		word_arrays[i] = twelfth_raw;
		word_arrays_sizes[i]=twelfth_raw_len;	
	    }
		if (g_strcmp0(word_str_lower,"thirteenth")==0) {		
		word_arrays[i] = (unsigned char*)malloc(thirteenth_raw_len * sizeof(unsigned char));
		word_arrays[i] = thirteenth_raw;;
		word_arrays_sizes[i]=thirteenth_raw_len;	
	    }
	    if (g_strcmp0(word_str_lower,"fourteenth")==0) {		
		word_arrays[i] = (unsigned char*)malloc(fourteenth_raw_len * sizeof(unsigned char));
		word_arrays[i] = fourteenth_raw;
		word_arrays_sizes[i]=fourteenth_raw_len;	
	    }		
		if (g_strcmp0(word_str_lower,"fifteenth")==0) {		
		word_arrays[i] = (unsigned char*)malloc(fifteenth_raw_len * sizeof(unsigned char));
		word_arrays[i] = fifteenth_raw;;
		word_arrays_sizes[i]=fifteenth_raw_len;	
	    }
	    if (g_strcmp0(word_str_lower,"sixteenth")==0) {		
		word_arrays[i] = (unsigned char*)malloc(sixteenth_raw_len * sizeof(unsigned char));
		word_arrays[i] = sixteenth_raw;
		word_arrays_sizes[i]=sixteenth_raw_len;	
	    }
	    if (g_strcmp0(word_str_lower,"seventeenth")==0) {		
		word_arrays[i] = (unsigned char*)malloc(seventeenth_raw_len * sizeof(unsigned char));
		word_arrays[i] = seventeenth_raw;
		word_arrays_sizes[i]=seventeenth_raw_len;	
	    }
	    if (g_strcmp0(word_str_lower,"eighteenth")==0) {		
		word_arrays[i] = (unsigned char*)malloc(eighteenth_raw_len * sizeof(unsigned char));
		word_arrays[i] = eighteenth_raw;
		word_arrays_sizes[i]=eighteenth_raw_len;	
	    }
	    if (g_strcmp0(word_str_lower,"nineteenth")==0) {		
		word_arrays[i] = (unsigned char*)malloc(nineteenth_raw_len * sizeof(unsigned char));
		word_arrays[i] = nineteenth_raw;
		word_arrays_sizes[i]=nineteenth_raw_len;	
	    }						
		if (g_strcmp0(word_str_lower,"twentieth")==0) {		
		word_arrays[i] = (unsigned char*)malloc(twentieth_raw_len * sizeof(unsigned char));
		word_arrays[i] = twentieth_raw;
		word_arrays_sizes[i]=twentieth_raw_len;	
	    }
	    if (g_strcmp0(word_str_lower,"twentyfirst")==0) {		
		word_arrays[i] = (unsigned char*)malloc(twentyfirst_raw_len * sizeof(unsigned char));
		word_arrays[i] = twentyfirst_raw;
		word_arrays_sizes[i]=twentyfirst_raw_len;	
	    }		
		if (g_strcmp0(word_str_lower,"twentysecond")==0) {		
		word_arrays[i] = (unsigned char*)malloc(twentysecond_raw_len * sizeof(unsigned char));
		word_arrays[i] = twentysecond_raw;
		word_arrays_sizes[i]=twentysecond_raw_len;	
	    }
	    if (g_strcmp0(word_str_lower,"twentythird")==0) {		
		word_arrays[i] = (unsigned char*)malloc(twentythird_raw_len * sizeof(unsigned char));
		word_arrays[i] = twentythird_raw;
		word_arrays_sizes[i]=twentythird_raw_len;	
	    }
	    if (g_strcmp0(word_str_lower,"twentyfourth")==0) {		
		word_arrays[i] = (unsigned char*)malloc(twentyfourth_raw_len * sizeof(unsigned char));
		word_arrays[i] = twentyfourth_raw;
		word_arrays_sizes[i]=twentyfourth_raw_len;	
	    }
	    if (g_strcmp0(word_str_lower,"twentyfifth")==0) {		
		word_arrays[i] = (unsigned char*)malloc(twentyfifth_raw_len * sizeof(unsigned char));
		word_arrays[i] = twentyfifth_raw;
		word_arrays_sizes[i]=twentyfifth_raw_len;	
	    }		
		if (g_strcmp0(word_str_lower,"twentysixth")==0) {		
		word_arrays[i] = (unsigned char*)malloc(twentysixth_raw_len * sizeof(unsigned char));
		word_arrays[i] = twentysixth_raw;
		word_arrays_sizes[i]=twentysixth_raw_len;	
	    }
	    if (g_strcmp0(word_str_lower,"twentyseventh")==0) {		
		word_arrays[i] = (unsigned char*)malloc(twentyseventh_raw_len * sizeof(unsigned char));
		word_arrays[i] = twentyseventh_raw;
		word_arrays_sizes[i]=twentyseventh_raw_len;	
	    }		
		if (g_strcmp0(word_str_lower,"twentyeighth")==0) {		
		word_arrays[i] = (unsigned char*)malloc(twentyeighth_raw_len * sizeof(unsigned char));
		word_arrays[i] = twentyeighth_raw;
		word_arrays_sizes[i]=twentyeighth_raw_len;	
	    }
	    if (g_strcmp0(word_str_lower,"twentynineth")==0) {		
		word_arrays[i] = (unsigned char*)malloc(twentynineth_raw_len * sizeof(unsigned char));
		word_arrays[i] = twentynineth_raw;
		word_arrays_sizes[i]=twentynineth_raw_len;	
	    }
	    if (g_strcmp0(word_str_lower,"thirtieth")==0) {	
		word_arrays[i] = (unsigned char*)malloc(thirtieth_raw_len * sizeof(unsigned char));
		word_arrays[i] = thirtieth_raw;
		word_arrays_sizes[i]=thirtieth_raw_len;	
	    }
	    if (g_strcmp0(word_str_lower,"thirtyfirst")==0) {
		word_arrays[i] = (unsigned char*)malloc(thirtyfirst_raw_len * sizeof(unsigned char));
		word_arrays[i] = thirtyfirst_raw;
		word_arrays_sizes[i]=thirtyfirst_raw_len;	
	    }						
		
		if (g_strcmp0(word_str_lower,"january")==0) {		
		word_arrays[i] = (unsigned char*)malloc(january_raw_len * sizeof(unsigned char));
		word_arrays[i] = january_raw;
		word_arrays_sizes[i]=january_raw_len;	
	    }
	    if (g_strcmp0(word_str_lower,"february")==0) {		
		word_arrays[i] = (unsigned char*)malloc(february_raw_len * sizeof(unsigned char));
		word_arrays[i] = february_raw;
		word_arrays_sizes[i]=february_raw_len;	
	    }		
		if (g_strcmp0(word_str_lower,"march")==0) {		
		word_arrays[i] = (unsigned char*)malloc(march_raw_len * sizeof(unsigned char));
		word_arrays[i] = march_raw;
		word_arrays_sizes[i]=march_raw_len;	
	    }	
		if (g_strcmp0(word_str_lower,"april")==0) {		
		word_arrays[i] = (unsigned char*)malloc(april_raw_len * sizeof(unsigned char));
		word_arrays[i] = april_raw;
		word_arrays_sizes[i]=april_raw_len;	
	    }	
		if (g_strcmp0(word_str_lower,"may")==0) {		
		word_arrays[i] = (unsigned char*)malloc(may_raw_len * sizeof(unsigned char));
		word_arrays[i] = may_raw;
		word_arrays_sizes[i]=may_raw_len;	
	    }
	    if (g_strcmp0(word_str_lower,"june")==0) {		
		word_arrays[i] = (unsigned char*)malloc(june_raw_len * sizeof(unsigned char));
		word_arrays[i] = june_raw;
		word_arrays_sizes[i]=june_raw_len;	
	    }		
		if (g_strcmp0(word_str_lower,"july")==0) {		
		word_arrays[i] = (unsigned char*)malloc(july_raw_len * sizeof(unsigned char));
		word_arrays[i] = july_raw;
		word_arrays_sizes[i]=july_raw_len;	
	    }	
		if (g_strcmp0(word_str_lower,"august")==0) {		
		word_arrays[i] = (unsigned char*)malloc(august_raw_len * sizeof(unsigned char));
		word_arrays[i] = august_raw;
		word_arrays_sizes[i]=august_raw_len;	
	    }
	    if (g_strcmp0(word_str_lower,"september")==0) {		
		word_arrays[i] = (unsigned char*)malloc(september_raw_len * sizeof(unsigned char));
		word_arrays[i] = september_raw;
		word_arrays_sizes[i]=september_raw_len;	
	    }
	    if (g_strcmp0(word_str_lower,"october")==0) {		
		word_arrays[i] = (unsigned char*)malloc(october_raw_len * sizeof(unsigned char));
		word_arrays[i] = october_raw;
		word_arrays_sizes[i]=october_raw_len;	
	    }
	    if (g_strcmp0(word_str_lower,"november")==0) {		
		word_arrays[i] = (unsigned char*)malloc(november_raw_len * sizeof(unsigned char));
		word_arrays[i] = november_raw;
		word_arrays_sizes[i]=november_raw_len;	
	    }
	    if (g_strcmp0(word_str_lower,"december")==0) {		
		word_arrays[i] = (unsigned char*)malloc(december_raw_len * sizeof(unsigned char));
		word_arrays[i] = december_raw;
		word_arrays_sizes[i]=december_raw_len;	
	    }					
		
	    //cardinals
	    if (g_strcmp0(word_str_lower,"one")==0) {		
		word_arrays[i] = (unsigned char*)malloc(one_raw_len * sizeof(unsigned char));
		word_arrays[i] = one_raw;
		word_arrays_sizes[i]=one_raw_len;
	    }
	     if (g_strcmp0(word_str_lower,"two")==0) {		
		word_arrays[i] = (unsigned char*)malloc(two_raw_len * sizeof(unsigned char));
		word_arrays[i] = two_raw;
		word_arrays_sizes[i]=two_raw_len;
	    }
	     if (g_strcmp0(word_str_lower,"three")==0) {		
		word_arrays[i] = (unsigned char*)malloc(three_raw_len * sizeof(unsigned char));
		word_arrays[i] = three_raw;
		word_arrays_sizes[i]=three_raw_len;
	    }
	     if (g_strcmp0(word_str_lower,"four")==0) {		
		word_arrays[i] = (unsigned char*)malloc(four_raw_len * sizeof(unsigned char));
		word_arrays[i] = four_raw;
		word_arrays_sizes[i]=four_raw_len;
	    }
	     if (g_strcmp0(word_str_lower,"five")==0) {		
		word_arrays[i] = (unsigned char*)malloc(five_raw_len * sizeof(unsigned char));
		word_arrays[i] = five_raw;
		word_arrays_sizes[i]=five_raw_len;
	    }
	     if (g_strcmp0(word_str_lower,"six")==0) {		
		word_arrays[i] = (unsigned char*)malloc(six_raw_len * sizeof(unsigned char));
		word_arrays[i] = six_raw;
		word_arrays_sizes[i]=six_raw_len;
	    }
	     if (g_strcmp0(word_str_lower,"seven")==0) {		
		word_arrays[i] = (unsigned char*)malloc(seven_raw_len * sizeof(unsigned char));
		word_arrays[i] = seven_raw;
		word_arrays_sizes[i]=seven_raw_len;
	    }
	     if (g_strcmp0(word_str_lower,"eight")==0) {		
		word_arrays[i] = (unsigned char*)malloc(eight_raw_len * sizeof(unsigned char));
		word_arrays[i] = eight_raw;
		word_arrays_sizes[i]=eight_raw_len;
	    }
	     if (g_strcmp0(word_str_lower,"nine")==0) {		
		word_arrays[i] = (unsigned char*)malloc(nine_raw_len * sizeof(unsigned char));
		word_arrays[i] = nine_raw;
		word_arrays_sizes[i]=nine_raw_len;
	    }
	     if (g_strcmp0(word_str_lower,"ten")==0) {		
		word_arrays[i] = (unsigned char*)malloc(ten_raw_len * sizeof(unsigned char));
		word_arrays[i] = ten_raw;
		word_arrays_sizes[i]=ten_raw_len;
	    }
	     if (g_strcmp0(word_str_lower,"eleven")==0) {		
		word_arrays[i] = (unsigned char*)malloc(eleven_raw_len * sizeof(unsigned char));
		word_arrays[i] = eleven_raw;
		word_arrays_sizes[i]=eleven_raw_len;
	    }
	     if (g_strcmp0(word_str_lower,"twelve")==0) {
		word_arrays[i] = (unsigned char*)malloc(twelve_raw_len * sizeof(unsigned char));
		word_arrays[i] = twelve_raw;
		word_arrays_sizes[i]=twelve_raw_len;
	    }
	     if (g_strcmp0(word_str_lower,"thirteen")==0) {		
		word_arrays[i] = (unsigned char*)malloc(thirteen_raw_len * sizeof(unsigned char));
		word_arrays[i] = thirteen_raw;
		word_arrays_sizes[i]=thirteen_raw_len;
	    }
	     if (g_strcmp0(word_str_lower,"fourteen")==0) {		
		word_arrays[i] = (unsigned char*)malloc(fourteen_raw_len * sizeof(unsigned char));
		word_arrays[i] = fourteen_raw;
		word_arrays_sizes[i]=fourteen_raw_len;
	    }	
	     if (g_strcmp0(word_str_lower,"fifteen")==0) {		
		word_arrays[i] = (unsigned char*)malloc(fifteen_raw_len * sizeof(unsigned char));
		word_arrays[i] = fifteen_raw;
		word_arrays_sizes[i]=fifteen_raw_len;
	    }
	     if (g_strcmp0(word_str_lower,"sixteen")==0) {		
		word_arrays[i] = (unsigned char*)malloc(sixteen_raw_len * sizeof(unsigned char));
		word_arrays[i] = sixteen_raw;
		word_arrays_sizes[i]=sixteen_raw_len;
	    }
	     if (g_strcmp0(word_str_lower,"seventeen")==0) {		
		word_arrays[i] = (unsigned char*)malloc(seventeen_raw_len * sizeof(unsigned char));
		word_arrays[i] = seventeen_raw;
		word_arrays_sizes[i]=seventeen_raw_len;
	    }
	     if (g_strcmp0(word_str_lower,"eighteen")==0) {		
		word_arrays[i] = (unsigned char*)malloc(eighteen_raw_len * sizeof(unsigned char));
		word_arrays[i] = eighteen_raw;
		word_arrays_sizes[i]=eighteen_raw_len;
	    }
	     if (g_strcmp0(word_str_lower,"nineteen")==0) {		
		word_arrays[i] = (unsigned char*)malloc(nineteen_raw_len * sizeof(unsigned char));
		word_arrays[i] = nineteen_raw;
		word_arrays_sizes[i]=nineteen_raw_len;
	    }
	     if (g_strcmp0(word_str_lower,"twenty")==0) {		
		word_arrays[i] = (unsigned char*)malloc(twenty_raw_len * sizeof(unsigned char));
		word_arrays[i] = twenty_raw;
		word_arrays_sizes[i]=twenty_raw_len;
	    }
	     if (g_strcmp0(word_str_lower,"twentyone")==0) {		
		word_arrays[i] = (unsigned char*)malloc(twentyone_raw_len * sizeof(unsigned char));
		word_arrays[i] = twentyone_raw;
		word_arrays_sizes[i]=twentyone_raw_len;
	    }
	     if (g_strcmp0(word_str_lower,"twentytwo")==0) {		
		word_arrays[i] = (unsigned char*)malloc(twentytwo_raw_len * sizeof(unsigned char));
		word_arrays[i] = twentytwo_raw;
		word_arrays_sizes[i]=twentytwo_raw_len;
	    }
	     if (g_strcmp0(word_str_lower,"twentythree")==0) {		
		word_arrays[i] = (unsigned char*)malloc(twentythree_raw_len * sizeof(unsigned char));
		word_arrays[i] = twentythree_raw;
		word_arrays_sizes[i]=twentythree_raw_len;
	    }
	     if (g_strcmp0(word_str_lower,"twentyfour")==0) {
		word_arrays[i] = (unsigned char*)malloc(twentyfour_raw_len * sizeof(unsigned char));
		word_arrays[i] = twentyfour_raw;
		word_arrays_sizes[i]=twentyfour_raw_len;
	    }
	     if (g_strcmp0(word_str_lower,"twentyfive")==0) {		
		word_arrays[i] = (unsigned char*)malloc(twentyfive_raw_len * sizeof(unsigned char));
		word_arrays[i] = twentyfive_raw;
		word_arrays_sizes[i]=twentyfive_raw_len;
	    }
	     if (g_strcmp0(word_str_lower,"twentysix")==0) {		
		word_arrays[i] = (unsigned char*)malloc(twentysix_raw_len * sizeof(unsigned char));
		word_arrays[i] = twentysix_raw;
		word_arrays_sizes[i]=twentysix_raw_len;
	    }	
	     if (g_strcmp0(word_str_lower,"twentyseven")==0) {		
		word_arrays[i] = (unsigned char*)malloc(twentyseven_raw_len * sizeof(unsigned char));
		word_arrays[i] = twentyseven_raw;
		word_arrays_sizes[i]=twentyseven_raw_len;
	    }
	     if (g_strcmp0(word_str_lower,"twentyeight")==0) {		
		word_arrays[i] = (unsigned char*)malloc(twentyeight_raw_len * sizeof(unsigned char));
		word_arrays[i] = twentyeight_raw;
		word_arrays_sizes[i]=twentyeight_raw_len;
	    }
	     if (g_strcmp0(word_str_lower,"twentynine")==0) {		
		word_arrays[i] = (unsigned char*)malloc(twentynine_raw_len * sizeof(unsigned char));
		word_arrays[i] = twentynine_raw;
		word_arrays_sizes[i]=twentynine_raw_len;
	    }
	     if (g_strcmp0(word_str_lower,"thirty")==0) {		
		word_arrays[i] = (unsigned char*)malloc(thirty_raw_len * sizeof(unsigned char));
		word_arrays[i] = thirty_raw;
		word_arrays_sizes[i]=thirty_raw_len;
	    }
	     if (g_strcmp0(word_str_lower,"thirtyone")==0) {		
		word_arrays[i] = (unsigned char*)malloc(thirtyone_raw_len * sizeof(unsigned char));
		word_arrays[i] = thirtyone_raw;
		word_arrays_sizes[i]=thirtyone_raw_len;
	    }
	     if (g_strcmp0(word_str_lower,"thirtytwo")==0) {		
		word_arrays[i] = (unsigned char*)malloc(thirtytwo_raw_len * sizeof(unsigned char));
		word_arrays[i] = thirtytwo_raw;
		word_arrays_sizes[i]=thirtytwo_raw_len;
	    }
	     if (g_strcmp0(word_str_lower,"thirtythree")==0) {		
		word_arrays[i] = (unsigned char*)malloc(thirtythree_raw_len * sizeof(unsigned char));
		word_arrays[i] = thirtythree_raw;
		word_arrays_sizes[i]=thirtythree_raw_len;
	    }
	     if (g_strcmp0(word_str_lower,"thirtyfour")==0) {		
		word_arrays[i] = (unsigned char*)malloc(thirtyfour_raw_len * sizeof(unsigned char));
		word_arrays[i] = thirtyfour_raw;
		word_arrays_sizes[i]=thirtyfour_raw_len;
	    }
	     if (g_strcmp0(word_str_lower,"thirtyfive")==0) {		
		word_arrays[i] = (unsigned char*)malloc(thirtyfive_raw_len * sizeof(unsigned char));
		word_arrays[i] = thirtyfive_raw;
		word_arrays_sizes[i]=thirtyfive_raw_len;
	    }
	     if (g_strcmp0(word_str_lower,"thirtysix")==0) {		
		word_arrays[i] = (unsigned char*)malloc(thirtysix_raw_len * sizeof(unsigned char));
		word_arrays[i] = thirtysix_raw;
		word_arrays_sizes[i]=thirtysix_raw_len;
	    }
	     if (g_strcmp0(word_str_lower,"thirtyseven")==0) {		
		word_arrays[i] = (unsigned char*)malloc(thirtyseven_raw_len * sizeof(unsigned char));
		word_arrays[i] = thirtyseven_raw;
		word_arrays_sizes[i]=thirtyseven_raw_len;
	    }
	     if (g_strcmp0(word_str_lower,"thirtyeight")==0) {		
		word_arrays[i] = (unsigned char*)malloc(thirtyeight_raw_len * sizeof(unsigned char));
		word_arrays[i] = thirtyeight_raw;
		word_arrays_sizes[i]=thirtyeight_raw_len;
	    }
	     if (g_strcmp0(word_str_lower,"thirtynine")==0) {		
		word_arrays[i] = (unsigned char*)malloc(thirtynine_raw_len * sizeof(unsigned char));
		word_arrays[i] = thirtynine_raw;
		word_arrays_sizes[i]=thirtynine_raw_len;
	    }
	     if (g_strcmp0(word_str_lower,"forty")==0) {		
		word_arrays[i] = (unsigned char*)malloc(forty_raw_len * sizeof(unsigned char));
		word_arrays[i] = forty_raw;
		word_arrays_sizes[i]=forty_raw_len;
	    }
	     if (g_strcmp0(word_str_lower,"fortyone")==0) {		
		word_arrays[i] = (unsigned char*)malloc(fortyone_raw_len * sizeof(unsigned char));
		word_arrays[i] = fortyone_raw;
		word_arrays_sizes[i]=fortyone_raw_len;
	    }
	     if (g_strcmp0(word_str_lower,"fortytwo")==0) {		
		word_arrays[i] = (unsigned char*)malloc(fortytwo_raw_len * sizeof(unsigned char));
		word_arrays[i] = fortytwo_raw;
		word_arrays_sizes[i]=fortythree_raw_len;
	    }
	     if (g_strcmp0(word_str_lower,"fortythree")==0) {		
		word_arrays[i] = (unsigned char*)malloc(fortythree_raw_len * sizeof(unsigned char));
		word_arrays[i] = fortythree_raw;
		word_arrays_sizes[i]=fortythree_raw_len;
	    }
	     if (g_strcmp0(word_str_lower,"fortyfour")==0) {		
		word_arrays[i] = (unsigned char*)malloc(fortyfour_raw_len * sizeof(unsigned char));
		word_arrays[i] = fortyfour_raw;
		word_arrays_sizes[i]=fortyfour_raw_len;
	    }
	     if (g_strcmp0(word_str_lower,"fortyfive")==0) {		
		word_arrays[i] = (unsigned char*)malloc(fortyfive_raw_len * sizeof(unsigned char));
		word_arrays[i] = fortyfive_raw;
		word_arrays_sizes[i]=fortyfive_raw_len;
	    }
	     if (g_strcmp0(word_str_lower,"fortysix")==0) {		
		word_arrays[i] = (unsigned char*)malloc(fortysix_raw_len * sizeof(unsigned char));
		word_arrays[i] = fortysix_raw;
		word_arrays_sizes[i]=fortysix_raw_len;
	    }
	     if (g_strcmp0(word_str_lower,"fortyseven")==0) {		
		word_arrays[i] = (unsigned char*)malloc(fortyseven_raw_len * sizeof(unsigned char));
		word_arrays[i] = fortyseven_raw;
		word_arrays_sizes[i]=fortyseven_raw_len;
	    }
	     if (g_strcmp0(word_str_lower,"fortyeight")==0) {		
		word_arrays[i] = (unsigned char*)malloc(fortyeight_raw_len * sizeof(unsigned char));
		word_arrays[i] = fortyeight_raw;
		word_arrays_sizes[i]=fortyeight_raw_len;
	    }
	     if (g_strcmp0(word_str_lower,"fortynine")==0) {		
		word_arrays[i] = (unsigned char*)malloc(fortynine_raw_len * sizeof(unsigned char));
		word_arrays[i] = fortynine_raw;
		word_arrays_sizes[i]=fortynine_raw_len;
	    }
	     if (g_strcmp0(word_str_lower,"fifty")==0) {		
		word_arrays[i] = (unsigned char*)malloc(fifty_raw_len * sizeof(unsigned char));
		word_arrays[i] = fifty_raw;
		word_arrays_sizes[i]=fifty_raw_len;
	    }																																																																																																
	      if (g_strcmp0(word_str_lower,"fiftyone")==0) {		
		word_arrays[i] = (unsigned char*)malloc(fiftyone_raw_len * sizeof(unsigned char));
		word_arrays[i] = fiftyone_raw;
		word_arrays_sizes[i]=fiftyone_raw_len;
	    }
	      if (g_strcmp0(word_str_lower,"fiftytwo")==0) {		
		word_arrays[i] = (unsigned char*)malloc(fiftytwo_raw_len * sizeof(unsigned char));
		word_arrays[i] = fiftytwo_raw;
		word_arrays_sizes[i]=fiftytwo_raw_len;
	    }
	      if (g_strcmp0(word_str_lower,"fiftythree")==0) {		
		word_arrays[i] = (unsigned char*)malloc(fiftythree_raw_len * sizeof(unsigned char));
		word_arrays[i] = fiftythree_raw;
		word_arrays_sizes[i]=fiftythree_raw_len;
	    }
	      if (g_strcmp0(word_str_lower,"fiftyfour")==0) {		
		word_arrays[i] = (unsigned char*)malloc(fiftyfour_raw_len * sizeof(unsigned char));
		word_arrays[i] = fiftyfour_raw;
		word_arrays_sizes[i]=fiftyfour_raw_len;
	    }
	    if (g_strcmp0(word_str_lower,"fiftyfive")==0) {		
		word_arrays[i] = (unsigned char*)malloc(fiftyfive_raw_len * sizeof(unsigned char));
		word_arrays[i] = fiftyfive_raw;
		word_arrays_sizes[i]=fiftyfive_raw_len;
	    }
	      if (g_strcmp0(word_str_lower,"fiftysix")==0) {		
		word_arrays[i] = (unsigned char*)malloc(fiftysix_raw_len * sizeof(unsigned char));
		word_arrays[i] = fiftysix_raw;
		word_arrays_sizes[i]=fiftysix_raw_len;
	    }
	      if (g_strcmp0(word_str_lower,"fiftyseven")==0) {		
		word_arrays[i] = (unsigned char*)malloc(fiftyseven_raw_len * sizeof(unsigned char));
		word_arrays[i] = fiftyseven_raw;
		word_arrays_sizes[i]=fiftyseven_raw_len;
	    }
	      if (g_strcmp0(word_str_lower,"fiftyeight")==0) {		
		word_arrays[i] = (unsigned char*)malloc(fiftyeight_raw_len * sizeof(unsigned char));
		word_arrays[i] = fiftyeight_raw;
		word_arrays_sizes[i]=fiftyeight_raw_len;
	    }
	      if (g_strcmp0(word_str_lower,"fiftynine")==0) {		
		word_arrays[i] = (unsigned char*)malloc(fiftynine_raw_len * sizeof(unsigned char));
		word_arrays[i] = fiftynine_raw;
		word_arrays_sizes[i]=fiftynine_raw_len;
	    }
	    
	    
	    //words
	     //A words   
	    if (g_strcmp0(word_str_lower,"all")==0) {		
		word_arrays[i] = (unsigned char*)malloc(all_raw_len * sizeof(unsigned char));
		word_arrays[i] = all_raw;
		word_arrays_sizes[i]=all_raw_len;
	    }
	    if (g_strcmp0(word_str_lower,"am")==0) {		
		word_arrays[i] = (unsigned char*)malloc(am_raw_len * sizeof(unsigned char));
		word_arrays[i] = am_raw;
		word_arrays_sizes[i]=am_raw_len;
	    }
	    if (g_strcmp0(word_str_lower,"activity")==0) {		
		word_arrays[i] = (unsigned char*)malloc(activity_raw_len * sizeof(unsigned char));
		word_arrays[i] = activity_raw;
		word_arrays_sizes[i]=activity_raw_len;	
	    }
	    if (g_strcmp0(word_str_lower,"anniversary")==0) {		
		word_arrays[i] = (unsigned char*)malloc(anniversary_raw_len * sizeof(unsigned char));
		word_arrays[i] = anniversary_raw;
		word_arrays_sizes[i]=anniversary_raw_len;	
	    }		
	    if (g_strcmp0(word_str_lower,"appointment")==0) {		
		word_arrays[i] = (unsigned char*)malloc(appointment_raw_len * sizeof(unsigned char));
		word_arrays[i] = appointment_raw;
		word_arrays_sizes[i]=appointment_raw_len;	
	    }	
	    
	    //B words
	    if (g_strcmp0(word_str_lower,"back")==0) {		
		word_arrays[i] = (unsigned char*)malloc(back_raw_len * sizeof(unsigned char));
		word_arrays[i] = back_raw;
		word_arrays_sizes[i]=back_raw_len;	
	    }
	    
	    if (g_strcmp0(word_str_lower,"bank")==0) {		
		word_arrays[i] = (unsigned char*)malloc(bank_raw_len * sizeof(unsigned char));
		word_arrays[i] = bank_raw;
		word_arrays_sizes[i]=bank_raw_len;	
	    }
	    if (g_strcmp0(word_str_lower,"begins")==0) {
		word_arrays[i] = (unsigned char*)malloc(begins_raw_len * sizeof(unsigned char));
		word_arrays[i] = begins_raw;
		word_arrays_sizes[i]=begins_raw_len;
		}	
	    
	    if (g_strcmp0(word_str_lower,"birthday")==0) {		
		word_arrays[i] = (unsigned char*)malloc(birthday_raw_len * sizeof(unsigned char));
		word_arrays[i] = birthday_raw;
		word_arrays_sizes[i]=birthday_raw_len;	
	    }
	     if (g_strcmp0(word_str_lower,"boxing")==0) {		
		word_arrays[i] = (unsigned char*)malloc(boxing_raw_len * sizeof(unsigned char));
		word_arrays[i] = boxing_raw;
		word_arrays_sizes[i]=boxing_raw_len;	
	    }
	    if (g_strcmp0(word_str_lower,"bst")==0) {		
		word_arrays[i] = (unsigned char*)malloc(bst_raw_len * sizeof(unsigned char));
		word_arrays[i] = bst_raw;
		word_arrays_sizes[i]=bst_raw_len;	
	    }		
	    //c-words
	   
	    if (g_strcmp0(word_str_lower,"cafe")==0) {		
		word_arrays[i] = (unsigned char*)malloc(cafe_raw_len * sizeof(unsigned char));
		word_arrays[i] = cafe_raw;
		word_arrays_sizes[i]=cafe_raw_len;	
	    }
	    if (g_strcmp0(word_str_lower,"car")==0) {		
		word_arrays[i] = (unsigned char*)malloc(car_raw_len * sizeof(unsigned char));
		word_arrays[i] = car_raw;
		word_arrays_sizes[i]=car_raw_len;	
	    }
	    if (g_strcmp0(word_str_lower,"christmas")==0) {		
		word_arrays[i] = (unsigned char*)malloc(christmas_raw_len * sizeof(unsigned char));
		word_arrays[i] = christmas_raw;
		word_arrays_sizes[i]=christmas_raw_len;	
	    }	
	    if (g_strcmp0(word_str_lower,"clocks")==0) {		
		word_arrays[i] = (unsigned char*)malloc(clocks_raw_len * sizeof(unsigned char));
		word_arrays[i] = clocks_raw;
		word_arrays_sizes[i]=clocks_raw_len;	
	    }			
	    
	    //D words
	    if (g_strcmp0(word_str_lower,"day")==0) {		
		word_arrays[i] = (unsigned char*)malloc(day_raw_len * sizeof(unsigned char));
		word_arrays[i] = day_raw;
		word_arrays_sizes[i]=day_raw_len;
	    }
	    if (g_strcmp0(word_str_lower,"dentist")==0) {		
		word_arrays[i] = (unsigned char*)malloc(dentist_raw_len * sizeof(unsigned char));
		word_arrays[i] = dentist_raw;
		word_arrays_sizes[i]=dentist_raw_len;	
	    }
	    if (g_strcmp0(word_str_lower,"doctor")==0) {		
		word_arrays[i] = (unsigned char*)malloc(doctor_raw_len * sizeof(unsigned char));
		word_arrays[i] = doctor_raw;
		word_arrays_sizes[i]=doctor_raw_len;	
	    }
	    
	    if (g_strcmp0(word_str_lower,"driver")==0) {		
		word_arrays[i] = (unsigned char*)malloc(driver_raw_len * sizeof(unsigned char));
		word_arrays[i] = driver_raw;
		word_arrays_sizes[i]=driver_raw_len;	
	    }
	    
	    // E words
	    if (g_strcmp0(word_str_lower,"easter")==0) {		
		word_arrays[i] = (unsigned char*)malloc(easter_raw_len * sizeof(unsigned char));
		word_arrays[i] = easter_raw;
		word_arrays_sizes[i]=easter_raw_len;
	    }
	    if (g_strcmp0(word_str_lower,"ends")==0) {		
		word_arrays[i] = (unsigned char*)malloc(ends_raw_len * sizeof(unsigned char));
		word_arrays[i] = ends_raw;
		word_arrays_sizes[i]=ends_raw_len;
	    }
	    if (g_strcmp0(word_str_lower,"event")==0) {		
		word_arrays[i] = (unsigned char*)malloc(event_raw_len * sizeof(unsigned char));
		word_arrays[i] = event_raw;
		word_arrays_sizes[i]=event_raw_len;
	    }
	    if (g_strcmp0(word_str_lower,"events")==0) {		
		word_arrays[i] = (unsigned char*)malloc(events_raw_len * sizeof(unsigned char));
		word_arrays[i] = events_raw;
		word_arrays_sizes[i]=events_raw_len;
	    }
	    //F words
	    
	    if (g_strcmp0(word_str_lower,"family")==0) {		
		word_arrays[i] = (unsigned char*)malloc(family_raw_len * sizeof(unsigned char));
		word_arrays[i] = family_raw;
		word_arrays_sizes[i]=family_raw_len;	
	    }
	    if (g_strcmp0(word_str_lower,"forward")==0) {		
		word_arrays[i] = (unsigned char*)malloc(forward_raw_len * sizeof(unsigned char));
		word_arrays[i] = forward_raw;
		word_arrays_sizes[i]=forward_raw_len;	
	    }
	    if (g_strcmp0(word_str_lower,"funeral")==0) {
		g_print("funeral detected\n");		
		word_arrays[i] = (unsigned char*)malloc(funeral_raw_len  * sizeof(unsigned char));
		word_arrays[i] = funeral_raw;
		word_arrays_sizes[i]=funeral_raw_len;			
	    }	
	    
	    //G-words
	    if (g_strcmp0(word_str_lower,"garden")==0) {		
		word_arrays[i] = (unsigned char*)malloc(garden_raw_len * sizeof(unsigned char));
		word_arrays[i] = garden_raw;
		word_arrays_sizes[i]=garden_raw_len;	
	    }		
	    //H words
	    if (g_strcmp0(word_str_lower,"high")==0) {
		word_arrays[i] = (unsigned char*)malloc(high_raw_len * sizeof(unsigned char));
		word_arrays[i] = high_raw;
		word_arrays_sizes[i]=high_raw_len;	
	    }	   
	    if (g_strcmp0(word_str_lower,"holiday")==0) {
		word_arrays[i] = (unsigned char*)malloc(holiday_raw_len * sizeof(unsigned char));
		word_arrays[i] = holiday_raw;
		word_arrays_sizes[i]=holiday_raw_len;	
	    }
	    if (g_strcmp0(word_str_lower,"hospital")==0) {
		word_arrays[i] = (unsigned char*)malloc(hospital_raw_len * sizeof(unsigned char));
		word_arrays[i] = hospital_raw;
		word_arrays_sizes[i]=hospital_raw_len;	
	    }
	    if (g_strcmp0(word_str_lower,"hour")==0) {
		word_arrays[i] = (unsigned char*)malloc(hour_raw_len * sizeof(unsigned char));
		word_arrays[i] = hour_raw;
		word_arrays_sizes[i]=hour_raw_len;	
	    }
	    //M words
	     
	    if (g_strcmp0(word_str_lower,"meeting")==0) {
		word_arrays[i] = (unsigned char*)malloc(meeting_raw_len * sizeof(unsigned char));
		word_arrays[i] = meeting_raw;
		word_arrays_sizes[i]=meeting_raw_len;	
	    } 
	     if (g_strcmp0(word_str_lower,"meetup")==0) {
		word_arrays[i] = (unsigned char*)malloc(meetup_raw_len * sizeof(unsigned char));
		word_arrays[i] = meetup_raw;
		word_arrays_sizes[i]=meetup_raw_len;	
	    } 
	    	    	    
	    //N words
	     if (g_strcmp0(word_str_lower,"new")==0) {
		word_arrays[i] = (unsigned char*)malloc(new_raw_len * sizeof(unsigned char));
		word_arrays[i] = new_raw;
		word_arrays_sizes[i]=new_raw_len;	
	    }
	    
	    if (g_strcmp0(word_str_lower,"no")==0) {
		word_arrays[i] = (unsigned char*)malloc(no_raw_len * sizeof(unsigned char));
		word_arrays[i] = no_raw;
		word_arrays_sizes[i]=no_raw_len;	
	    }
	     
	     //P words
	    if (g_strcmp0(word_str_lower,"payment")==0) {
		word_arrays[i] = (unsigned char*)malloc(payment_raw_len * sizeof(unsigned char));
		word_arrays[i] = payment_raw;
		word_arrays_sizes[i]=payment_raw_len;	
	    } 
	    
	    if (g_strcmp0(word_str_lower,"party")==0) {		
		word_arrays[i] = (unsigned char*)malloc(party_raw_len * sizeof(unsigned char));
		word_arrays[i] = party_raw;
		word_arrays_sizes[i]=party_raw_len;	
	    }
	    
	    if (g_strcmp0(word_str_lower,"pm")==0) {
		word_arrays[i] = (unsigned char*)malloc(pm_raw_len * sizeof(unsigned char));
		word_arrays[i] = pm_raw;
		word_arrays_sizes[i]=pm_raw_len;
	    }
	     if (g_strcmp0(word_str_lower,"priority")==0) {
		word_arrays[i] = (unsigned char*)malloc(priority_raw_len * sizeof(unsigned char));
		word_arrays[i] = priority_raw;
		word_arrays_sizes[i]=priority_raw_len;	
	    }
	    //R words
	    if (g_strcmp0(word_str_lower,"reminder")==0) {
		word_arrays[i] = (unsigned char*)malloc(reminder_raw_len * sizeof(unsigned char));
		word_arrays[i] = reminder_raw;
		word_arrays_sizes[i]=reminder_raw_len;	
	    }
	    if (g_strcmp0(word_str_lower,"restaurant")==0) {
		word_arrays[i] = (unsigned char*)malloc(restaurant_raw_len * sizeof(unsigned char));
		word_arrays[i] = restaurant_raw;
		word_arrays_sizes[i]=restaurant_raw_len;	
	    }
	    //S words
	    if (g_strcmp0(word_str_lower,"spring")==0) {
		word_arrays[i] = (unsigned char*)malloc(spring_raw_len * sizeof(unsigned char));
		word_arrays[i] = spring_raw;
		word_arrays_sizes[i]=spring_raw_len;
		}
			
	    
	    //T words
	    if (g_strcmp0(word_str_lower,"task")==0) {
		word_arrays[i] = (unsigned char*)malloc(task_raw_len * sizeof(unsigned char));
		word_arrays[i] = task_raw;
		word_arrays_sizes[i]=task_raw_len;	
	    }
	    if (g_strcmp0(word_str_lower,"today")==0) {
		word_arrays[i] = (unsigned char*)malloc(today_raw_len * sizeof(unsigned char));
		word_arrays[i] = today_raw;
		word_arrays_sizes[i]=today_raw_len;	
	    }
	    if (g_strcmp0(word_str_lower,"travel")==0) {
		word_arrays[i] = (unsigned char*)malloc(travel_raw_len * sizeof(unsigned char));
		word_arrays[i] = travel_raw;
		word_arrays_sizes[i]=travel_raw_len;	
	    }
	    //U words	    
	    if (g_strcmp0(word_str_lower,"upcoming")==0) {
		word_arrays[i] = (unsigned char*)malloc(upcoming_raw_len * sizeof(unsigned char));
		word_arrays[i] = upcoming_raw;
		word_arrays_sizes[i]=upcoming_raw_len;
	    }	
	    
	    //V words	    
	    if (g_strcmp0(word_str_lower,"visit")==0) {
		word_arrays[i] = (unsigned char*)malloc(visit_raw_len * sizeof(unsigned char));
		word_arrays[i] = visit_raw;
		word_arrays_sizes[i]=visit_raw_len;	
	    }
	    if (g_strcmp0(word_str_lower,"valentine")==0) {
		word_arrays[i] = (unsigned char*)malloc(valentine_raw_len * sizeof(unsigned char));
		word_arrays[i] = valentine_raw;
		word_arrays_sizes[i]=valentine_raw_len;	
	    }
	    //W words
	    if (g_strcmp0(word_str_lower,"work")==0) {
		word_arrays[i] = (unsigned char*)malloc(work_raw_len * sizeof(unsigned char));
		word_arrays[i] = work_raw;
		word_arrays_sizes[i]=work_raw_len;	
	    }
	     //Y words
	    if (g_strcmp0(word_str_lower,"year")==0) {
		word_arrays[i] = (unsigned char*)malloc(year_raw_len * sizeof(unsigned char));
		word_arrays[i] = year_raw;
		word_arrays_sizes[i]=year_raw_len;	
	    }																													
	
	}//for
		
	//concatenate using raw cat
	unsigned char *data = rawcat(word_arrays, word_arrays_sizes, word_number);	
	unsigned int data_len = get_merge_size(word_arrays_sizes,word_number);	
    gchar* raw_file ="/tmp/talkout.raw";
	FILE* f = fopen(raw_file, "w");
    fwrite(data, data_len, 1, f);
    fclose(f);
  
    //playraw(raw_file);
    
    GThread *thread_audio; 
	g_mutex_lock (&lock);
    thread_audio = g_thread_new(NULL, thread_playraw, raw_file);  
	g_thread_unref (thread_audio);
   		
	free(data);	
	
}
//======================================================================
static void speak_time(gint hour, gint min) 
{
	gchar* hour_str="";
	gchar* min_str="";
	gchar* ampm_str="";
	GList *speak_word_list = NULL;
	
	speak_word_list = g_list_append(speak_word_list, "the");
	speak_word_list = g_list_append(speak_word_list, "time");
	speak_word_list = g_list_append(speak_word_list, "is");
		
	if(m_12hour_format) {
	
	if(hour >=1 && hour<12) {
	//AM
	hour_str =get_cardinal_string(hour);
	
	ampm_str="am";
	speak_word_list = g_list_append(speak_word_list, hour_str);
	
	if (min ==0)
	{
		speak_word_list = g_list_append(speak_word_list, ampm_str);	
	}
	else if(min >0 && min <=9)
	{
		min_str=get_cardinal_string(min);
		speak_word_list = g_list_append(speak_word_list, "zero");
		speak_word_list = g_list_append(speak_word_list, min_str);
	    speak_word_list = g_list_append(speak_word_list, ampm_str);	
	}	
	else if(min>9)
	{
	min_str=get_cardinal_string(min);
	speak_word_list = g_list_append(speak_word_list, min_str);
	speak_word_list = g_list_append(speak_word_list, ampm_str);			    
	}			
	
	}
	else if(hour ==12) {
	//PM
	hour_str =get_cardinal_string(hour);
	min_str=get_cardinal_string(min);
	ampm_str="pm";
	speak_word_list = g_list_append(speak_word_list, hour_str);
	
	if (min ==0)
	{
		speak_word_list = g_list_append(speak_word_list, ampm_str);	
	}
	else if(min >0 && min <=9)
	{
		min_str=get_cardinal_string(min);
		speak_word_list = g_list_append(speak_word_list, "zero");
		speak_word_list = g_list_append(speak_word_list, min_str);
	    speak_word_list = g_list_append(speak_word_list, ampm_str);	
	}	
	else if(min>9)
	{
	min_str=get_cardinal_string(min);
	speak_word_list = g_list_append(speak_word_list, min_str);
	speak_word_list = g_list_append(speak_word_list, ampm_str);			    
	}			
		
	}
	else if (hour >=13 && hour<=23) {
	//PM
	hour=hour-12;
	hour_str =get_cardinal_string(hour);
	//min_str=get_cardinal_string(min);
	ampm_str="pm";
	speak_word_list = g_list_append(speak_word_list, hour_str);
	
	if (min ==0)
	{
		speak_word_list = g_list_append(speak_word_list, ampm_str);	
	}
	else if(min >0 && min <=9)
	{
		
		min_str=get_cardinal_string(min);
		speak_word_list = g_list_append(speak_word_list, "zero");
		speak_word_list = g_list_append(speak_word_list, min_str);
	    speak_word_list = g_list_append(speak_word_list, ampm_str);	
	}	
	else if(min>9)
	{
	min_str=get_cardinal_string(min);
	speak_word_list = g_list_append(speak_word_list, min_str);
	speak_word_list = g_list_append(speak_word_list, ampm_str);			    
	}			
				
	} //hour 13 to 23
			
	} //12hour format
	else
	{				
	hour_str =get_cardinal_string(hour);
	min_str=get_cardinal_string(min);
	speak_word_list = g_list_append(speak_word_list, hour_str);
	speak_word_list = g_list_append(speak_word_list, min_str);			    				
	} //24 hour format	
		
	gpointer word_list_pointer;
	gchar* word_str;
	gchar* word_str_lower;	
	gint word_number  =g_list_length(speak_word_list);
		
	//create word array using list size
	unsigned char *word_arrays[word_number]; 
	unsigned int word_arrays_sizes[word_number];
	//load hex words into array
	
	for(int i=0; i < word_number; i++)
	{
	word_list_pointer=g_list_nth_data(speak_word_list,i);
	word_str=(gchar *)word_list_pointer;
	//g_print("time: wordStr = %s\n", word_str);
	gchar* word_str_lower= g_ascii_strdown(word_str,-1);	//make sure lower	
		
	//load up arrays
	//dummy empty	
	word_arrays[i] = (unsigned char*)malloc(empty_raw_len * sizeof(unsigned char));
	word_arrays[i] = empty_raw;
	word_arrays_sizes[i]=empty_raw_len; 
	
	if (g_strcmp0(word_str_lower,"the")==0) {
	word_arrays[i] = (unsigned char*)malloc(the_raw_len * sizeof(unsigned char));
	word_arrays[i] = the_raw;
	word_arrays_sizes[i]=the_raw_len;	
	}
	
	if (g_strcmp0(word_str_lower,"time")==0) {
	word_arrays[i] = (unsigned char*)malloc(time_raw_len * sizeof(unsigned char));
	word_arrays[i] = time_raw;
	word_arrays_sizes[i]=time_raw_len;	
	}
	
	if (g_strcmp0(word_str_lower,"is")==0) {
	word_arrays[i] = (unsigned char*)malloc(is_raw_len * sizeof(unsigned char));
	word_arrays[i] = is_raw;
	word_arrays_sizes[i]=is_raw_len;	
	}
	//cardinals
	if (g_strcmp0(word_str_lower,"zero")==0) {
	word_arrays[i] = (unsigned char*)malloc(zero_raw_len * sizeof(unsigned char));
	word_arrays[i] = zero_raw;
	word_arrays_sizes[i]=zero_raw_len;
	}
	
	if (g_strcmp0(word_str_lower,"one")==0) {
	word_arrays[i] = (unsigned char*)malloc(one_raw_len * sizeof(unsigned char));
	word_arrays[i] = one_raw;
	word_arrays_sizes[i]=one_raw_len;
	}
	if (g_strcmp0(word_str_lower,"two")==0) {
	word_arrays[i] = (unsigned char*)malloc(two_raw_len * sizeof(unsigned char));
	word_arrays[i] = two_raw;
	word_arrays_sizes[i]=two_raw_len;
	}
	if (g_strcmp0(word_str_lower,"three")==0) {
	word_arrays[i] = (unsigned char*)malloc(three_raw_len * sizeof(unsigned char));
	word_arrays[i] = three_raw;
	word_arrays_sizes[i]=three_raw_len;
	}
	if (g_strcmp0(word_str_lower,"four")==0) {
	word_arrays[i] = (unsigned char*)malloc(four_raw_len * sizeof(unsigned char));
	word_arrays[i] = four_raw;
	word_arrays_sizes[i]=four_raw_len;
	}
	if (g_strcmp0(word_str_lower,"five")==0) {
	word_arrays[i] = (unsigned char*)malloc(five_raw_len * sizeof(unsigned char));
	word_arrays[i] = five_raw;
	word_arrays_sizes[i]=five_raw_len;
	}
	if (g_strcmp0(word_str_lower,"six")==0) {
	word_arrays[i] = (unsigned char*)malloc(six_raw_len * sizeof(unsigned char));
	word_arrays[i] = six_raw;
	word_arrays_sizes[i]=six_raw_len;
	}
	if (g_strcmp0(word_str_lower,"seven")==0) {
	word_arrays[i] = (unsigned char*)malloc(seven_raw_len * sizeof(unsigned char));
	word_arrays[i] = seven_raw;
	word_arrays_sizes[i]=seven_raw_len;
	}
	if (g_strcmp0(word_str_lower,"eight")==0) {
	word_arrays[i] = (unsigned char*)malloc(eight_raw_len * sizeof(unsigned char));
	word_arrays[i] = eight_raw;
	word_arrays_sizes[i]=eight_raw_len;
	}
	if (g_strcmp0(word_str_lower,"nine")==0) {
	word_arrays[i] = (unsigned char*)malloc(nine_raw_len * sizeof(unsigned char));
	word_arrays[i] = nine_raw;
	word_arrays_sizes[i]=nine_raw_len;
	}
	if (g_strcmp0(word_str_lower,"ten")==0) {
	word_arrays[i] = (unsigned char*)malloc(ten_raw_len * sizeof(unsigned char));
	word_arrays[i] = ten_raw;
	word_arrays_sizes[i]=ten_raw_len;
	}
	if (g_strcmp0(word_str_lower,"eleven")==0) {
	word_arrays[i] = (unsigned char*)malloc(eleven_raw_len * sizeof(unsigned char));
	word_arrays[i] = eleven_raw;
	word_arrays_sizes[i]=eleven_raw_len;
	}
	if (g_strcmp0(word_str_lower,"twelve")==0) {
	word_arrays[i] = (unsigned char*)malloc(twelve_raw_len * sizeof(unsigned char));
	word_arrays[i] = twelve_raw;
	word_arrays_sizes[i]=twelve_raw_len;
	}
	if (g_strcmp0(word_str_lower,"thirteen")==0) {
	word_arrays[i] = (unsigned char*)malloc(thirteen_raw_len * sizeof(unsigned char));
	word_arrays[i] = thirteen_raw;
	word_arrays_sizes[i]=thirteen_raw_len;
	}
	if (g_strcmp0(word_str_lower,"fourteen")==0) {
	word_arrays[i] = (unsigned char*)malloc(fourteen_raw_len * sizeof(unsigned char));
	word_arrays[i] = fourteen_raw;
	word_arrays_sizes[i]=fourteen_raw_len;
	}	
	if (g_strcmp0(word_str_lower,"fifteen")==0) {
	word_arrays[i] = (unsigned char*)malloc(fifteen_raw_len * sizeof(unsigned char));
	word_arrays[i] = fifteen_raw;
	word_arrays_sizes[i]=fifteen_raw_len;
	}
	if (g_strcmp0(word_str_lower,"sixteen")==0) {
	word_arrays[i] = (unsigned char*)malloc(sixteen_raw_len * sizeof(unsigned char));
	word_arrays[i] = sixteen_raw;
	word_arrays_sizes[i]=sixteen_raw_len;
	}
	if (g_strcmp0(word_str_lower,"seventeen")==0) {
	word_arrays[i] = (unsigned char*)malloc(seventeen_raw_len * sizeof(unsigned char));
	word_arrays[i] = seventeen_raw;
	word_arrays_sizes[i]=seventeen_raw_len;
	}
	if (g_strcmp0(word_str_lower,"eighteen")==0) {
	word_arrays[i] = (unsigned char*)malloc(eighteen_raw_len * sizeof(unsigned char));
	word_arrays[i] = eighteen_raw;
	word_arrays_sizes[i]=eighteen_raw_len;
	}
	if (g_strcmp0(word_str_lower,"nineteen")==0) {
	word_arrays[i] = (unsigned char*)malloc(nineteen_raw_len * sizeof(unsigned char));
	word_arrays[i] = nineteen_raw;
	word_arrays_sizes[i]=nineteen_raw_len;
	}
	if (g_strcmp0(word_str_lower,"twenty")==0) {
	word_arrays[i] = (unsigned char*)malloc(twenty_raw_len * sizeof(unsigned char));
	word_arrays[i] = twenty_raw;
	word_arrays_sizes[i]=twenty_raw_len;
	}
	if (g_strcmp0(word_str_lower,"twentyone")==0) {
	word_arrays[i] = (unsigned char*)malloc(twentyone_raw_len * sizeof(unsigned char));
	word_arrays[i] = twentyone_raw;
	word_arrays_sizes[i]=twentyone_raw_len;
	}
	if (g_strcmp0(word_str_lower,"twentytwo")==0) {
	word_arrays[i] = (unsigned char*)malloc(twentytwo_raw_len * sizeof(unsigned char));
	word_arrays[i] = twentytwo_raw;
	word_arrays_sizes[i]=twentytwo_raw_len;
	}
	if (g_strcmp0(word_str_lower,"twentythree")==0) {
	word_arrays[i] = (unsigned char*)malloc(twentythree_raw_len * sizeof(unsigned char));
	word_arrays[i] = twentythree_raw;
	word_arrays_sizes[i]=twentythree_raw_len;
	}
	if (g_strcmp0(word_str_lower,"twentyfour")==0) {
	word_arrays[i] = (unsigned char*)malloc(twentyfour_raw_len * sizeof(unsigned char));
	word_arrays[i] = twentyfour_raw;
	word_arrays_sizes[i]=twentyfour_raw_len;
	}
	if (g_strcmp0(word_str_lower,"twentyfive")==0) {
	word_arrays[i] = (unsigned char*)malloc(twentyfive_raw_len * sizeof(unsigned char));
	word_arrays[i] = twentyfive_raw;
	word_arrays_sizes[i]=twentyfive_raw_len;
	}
	if (g_strcmp0(word_str_lower,"twentysix")==0) {
	word_arrays[i] = (unsigned char*)malloc(twentysix_raw_len * sizeof(unsigned char));
	word_arrays[i] = twentysix_raw;
	word_arrays_sizes[i]=twentysix_raw_len;
	}	
	if (g_strcmp0(word_str_lower,"twentyseven")==0) {
	word_arrays[i] = (unsigned char*)malloc(twentyseven_raw_len * sizeof(unsigned char));
	word_arrays[i] = twentyseven_raw;
	word_arrays_sizes[i]=twentyseven_raw_len;
	}
	if (g_strcmp0(word_str_lower,"twentyeight")==0) {
	word_arrays[i] = (unsigned char*)malloc(twentyeight_raw_len * sizeof(unsigned char));
	word_arrays[i] = twentyeight_raw;
	word_arrays_sizes[i]=twentyeight_raw_len;
	}
	if (g_strcmp0(word_str_lower,"twentynine")==0) {
	word_arrays[i] = (unsigned char*)malloc(twentynine_raw_len * sizeof(unsigned char));
	word_arrays[i] = twentynine_raw;
	word_arrays_sizes[i]=twentynine_raw_len;
	}
	if (g_strcmp0(word_str_lower,"thirty")==0) {
	word_arrays[i] = (unsigned char*)malloc(thirty_raw_len * sizeof(unsigned char));
	word_arrays[i] = thirty_raw;
	word_arrays_sizes[i]=thirty_raw_len;
	}
	if (g_strcmp0(word_str_lower,"thirtyone")==0) {
	word_arrays[i] = (unsigned char*)malloc(thirtyone_raw_len * sizeof(unsigned char));
	word_arrays[i] = thirtyone_raw;
	word_arrays_sizes[i]=thirtyone_raw_len;
	}
	if (g_strcmp0(word_str_lower,"thirtytwo")==0) {
	word_arrays[i] = (unsigned char*)malloc(thirtytwo_raw_len * sizeof(unsigned char));
	word_arrays[i] = thirtytwo_raw;
	word_arrays_sizes[i]=thirtytwo_raw_len;
	}
	if (g_strcmp0(word_str_lower,"thirtythree")==0) {
	word_arrays[i] = (unsigned char*)malloc(thirtythree_raw_len * sizeof(unsigned char));
	word_arrays[i] = thirtythree_raw;
	word_arrays_sizes[i]=thirtythree_raw_len;
	}
	if (g_strcmp0(word_str_lower,"thirtyfour")==0) {
	word_arrays[i] = (unsigned char*)malloc(thirtyfour_raw_len * sizeof(unsigned char));
	word_arrays[i] = thirtyfour_raw;
	word_arrays_sizes[i]=thirtyfour_raw_len;
	}
	if (g_strcmp0(word_str_lower,"thirtyfive")==0) {
	word_arrays[i] = (unsigned char*)malloc(thirtyfive_raw_len * sizeof(unsigned char));
	word_arrays[i] = thirtyfive_raw;
	word_arrays_sizes[i]=thirtyfive_raw_len;
	}
	if (g_strcmp0(word_str_lower,"thirtysix")==0) {
	word_arrays[i] = (unsigned char*)malloc(thirtysix_raw_len * sizeof(unsigned char));
	word_arrays[i] = thirtysix_raw;
	word_arrays_sizes[i]=thirtysix_raw_len;
	}
	if (g_strcmp0(word_str_lower,"thirtyseven")==0) {
	word_arrays[i] = (unsigned char*)malloc(thirtyseven_raw_len * sizeof(unsigned char));
	word_arrays[i] = thirtyseven_raw;
	word_arrays_sizes[i]=thirtyseven_raw_len;
	}
	if (g_strcmp0(word_str_lower,"thirtyeight")==0) {
	word_arrays[i] = (unsigned char*)malloc(thirtyeight_raw_len * sizeof(unsigned char));
	word_arrays[i] = thirtyeight_raw;
	word_arrays_sizes[i]=thirtyeight_raw_len;
	}
	if (g_strcmp0(word_str_lower,"thirtynine")==0) {
	word_arrays[i] = (unsigned char*)malloc(thirtynine_raw_len * sizeof(unsigned char));
	word_arrays[i] = thirtynine_raw;
	word_arrays_sizes[i]=thirtynine_raw_len;
	}
	if (g_strcmp0(word_str_lower,"forty")==0) {
	word_arrays[i] = (unsigned char*)malloc(forty_raw_len * sizeof(unsigned char));
	word_arrays[i] = forty_raw;
	word_arrays_sizes[i]=forty_raw_len;
	}
	if (g_strcmp0(word_str_lower,"fortyone")==0) {
	word_arrays[i] = (unsigned char*)malloc(fortyone_raw_len * sizeof(unsigned char));
	word_arrays[i] = fortyone_raw;
	word_arrays_sizes[i]=fortyone_raw_len;
	}
	if (g_strcmp0(word_str_lower,"fortytwo")==0) {
	word_arrays[i] = (unsigned char*)malloc(fortytwo_raw_len * sizeof(unsigned char));
	word_arrays[i] = fortytwo_raw;
	word_arrays_sizes[i]=fortythree_raw_len;
	}
	if (g_strcmp0(word_str_lower,"fortythree")==0) {
	word_arrays[i] = (unsigned char*)malloc(fortythree_raw_len * sizeof(unsigned char));
	word_arrays[i] = fortythree_raw;
	word_arrays_sizes[i]=fortythree_raw_len;
	}
	if (g_strcmp0(word_str_lower,"fortyfour")==0) {
	word_arrays[i] = (unsigned char*)malloc(fortyfour_raw_len * sizeof(unsigned char));
	word_arrays[i] = fortyfour_raw;
	word_arrays_sizes[i]=fortyfour_raw_len;
	}
	if (g_strcmp0(word_str_lower,"fortyfive")==0) {
	word_arrays[i] = (unsigned char*)malloc(fortyfive_raw_len * sizeof(unsigned char));
	word_arrays[i] = fortyfive_raw;
	word_arrays_sizes[i]=fortyfive_raw_len;
	}
	if (g_strcmp0(word_str_lower,"fortysix")==0) {
	word_arrays[i] = (unsigned char*)malloc(fortysix_raw_len * sizeof(unsigned char));
	word_arrays[i] = fortysix_raw;
	word_arrays_sizes[i]=fortysix_raw_len;
	}
	if (g_strcmp0(word_str_lower,"fortyseven")==0) {
	word_arrays[i] = (unsigned char*)malloc(fortyseven_raw_len * sizeof(unsigned char));
	word_arrays[i] = fortyseven_raw;
	word_arrays_sizes[i]=fortyseven_raw_len;
	}
	if (g_strcmp0(word_str_lower,"fortyeight")==0) {
	word_arrays[i] = (unsigned char*)malloc(fortyeight_raw_len * sizeof(unsigned char));
	word_arrays[i] = fortyeight_raw;
	word_arrays_sizes[i]=fortyeight_raw_len;
	}
	if (g_strcmp0(word_str_lower,"fortynine")==0) {
	word_arrays[i] = (unsigned char*)malloc(fortynine_raw_len * sizeof(unsigned char));
	word_arrays[i] = fortynine_raw;
	word_arrays_sizes[i]=fortynine_raw_len;
	}
	if (g_strcmp0(word_str_lower,"fifty")==0) {
	word_arrays[i] = (unsigned char*)malloc(fifty_raw_len * sizeof(unsigned char));
	word_arrays[i] = fifty_raw;
	word_arrays_sizes[i]=fifty_raw_len;
	}																																																																																																
	if (g_strcmp0(word_str_lower,"fiftyone")==0) {
	word_arrays[i] = (unsigned char*)malloc(fiftyone_raw_len * sizeof(unsigned char));
	word_arrays[i] = fiftyone_raw;
	word_arrays_sizes[i]=fiftyone_raw_len;
	}
	if (g_strcmp0(word_str_lower,"fiftytwo")==0) {
	word_arrays[i] = (unsigned char*)malloc(fiftytwo_raw_len * sizeof(unsigned char));
	word_arrays[i] = fiftytwo_raw;
	word_arrays_sizes[i]=fiftytwo_raw_len;
	}
	if (g_strcmp0(word_str_lower,"fiftythree")==0) {
	word_arrays[i] = (unsigned char*)malloc(fiftythree_raw_len * sizeof(unsigned char));
	word_arrays[i] = fiftythree_raw;
	word_arrays_sizes[i]=fiftythree_raw_len;
	}
	if (g_strcmp0(word_str_lower,"fiftyfour")==0) {
	word_arrays[i] = (unsigned char*)malloc(fiftyfour_raw_len * sizeof(unsigned char));
	word_arrays[i] = fiftyfour_raw;
	word_arrays_sizes[i]=fiftyfour_raw_len;
	}
	if (g_strcmp0(word_str_lower,"fiftyfive")==0) {
	word_arrays[i] = (unsigned char*)malloc(fiftyfive_raw_len * sizeof(unsigned char));
	word_arrays[i] = fiftyfive_raw;
	word_arrays_sizes[i]=fiftyfive_raw_len;
	}
	if (g_strcmp0(word_str_lower,"fiftysix")==0) {
	word_arrays[i] = (unsigned char*)malloc(fiftysix_raw_len * sizeof(unsigned char));
	word_arrays[i] = fiftysix_raw;
	word_arrays_sizes[i]=fiftysix_raw_len;
	}
	if (g_strcmp0(word_str_lower,"fiftyseven")==0) {
	word_arrays[i] = (unsigned char*)malloc(fiftyseven_raw_len * sizeof(unsigned char));
	word_arrays[i] = fiftyseven_raw;
	word_arrays_sizes[i]=fiftyseven_raw_len;
	}
	if (g_strcmp0(word_str_lower,"fiftyeight")==0) {
	word_arrays[i] = (unsigned char*)malloc(fiftyeight_raw_len * sizeof(unsigned char));
	word_arrays[i] = fiftyeight_raw;
	word_arrays_sizes[i]=fiftyeight_raw_len;
	}
	if (g_strcmp0(word_str_lower,"fiftynine")==0) {
	word_arrays[i] = (unsigned char*)malloc(fiftynine_raw_len * sizeof(unsigned char));
	word_arrays[i] = fiftynine_raw;
	word_arrays_sizes[i]=fiftynine_raw_len;
	}
	
	//24 hour
	if (g_strcmp0(word_str_lower,"am")==0) {
	word_arrays[i] = (unsigned char*)malloc(am_raw_len * sizeof(unsigned char));
	word_arrays[i] = am_raw;
	word_arrays_sizes[i]=am_raw_len;
	}
	if (g_strcmp0(word_str_lower,"pm")==0) {
	word_arrays[i] = (unsigned char*)malloc(pm_raw_len * sizeof(unsigned char));
	word_arrays[i] = pm_raw;
	word_arrays_sizes[i]=pm_raw_len;
	}		
	} //for words
	
	//concatenate using raw cat
	unsigned char *data = rawcat(word_arrays, word_arrays_sizes, word_number);	
	unsigned int data_len = get_merge_size(word_arrays_sizes,word_number);
	
    gchar *raw_file ="/tmp/time.raw";     
	FILE* f = fopen(raw_file, "w");
    fwrite(data, data_len, 1, f);
    fclose(f);     
    //playraw(raw_file); 
    
    GThread *thread_audio; 
	g_mutex_lock (&lock);
    thread_audio = g_thread_new(NULL, thread_playraw, raw_file);  
	g_thread_unref (thread_audio);
      		
	free(data);	
	
}
//======================================================================
static void callbk_calendar_speaktime(GSimpleAction * action, GVariant *parameter, gpointer user_data)
{	
	GtkWidget *window = user_data;
	
	GDateTime *dt = g_date_time_new_now_local(); 
	gint hour =g_date_time_get_hour(dt);	
	gint min= g_date_time_get_minute(dt);	
	g_print("hour = %i\n",hour);
	g_print("min = %i\n",min);
	g_print("calling speak time\n");
	speak_time(hour,min);	
	
    g_date_time_unref (dt);
}
//======================================================================
static void show_notifications(GArray *evt_arry){
	
	gint evt_id;
	const gchar *summary_str;
	const gchar *location_str;
	const gchar *description_str;

	gint start_year;
	gint start_month;
	gint start_day;
	gint start_hour;
	gint start_min;

	gint end_year;
	gint end_month;
	gint end_day;
	gint end_hour;
	gint end_min;

	gint is_yearly;
	gint is_allday;
	gint is_priority;
	gint has_notification;
	gint has_reminder;
	gint reminder_min;
	
	// Display
	
    gchar *display_str = "";
	gchar *notify_id ="";

	// loop through the events array adding events to listbox store
	for (int i = 0; i < evt_arry->len; i++)	{

		CalendarEvent *evt = g_array_index(evt_arry, CalendarEvent *, i);
		g_object_get(evt, "eventid", &evt_id, NULL);
		g_object_get(evt, "summary", &summary_str, NULL);
		g_object_get(evt, "location", &location_str, NULL);
		g_object_get(evt, "description", &description_str, NULL);
		g_object_get(evt, "startyear", &start_year, NULL);
		g_object_get(evt, "startmonth", &start_month, NULL);
		g_object_get(evt, "startday", &start_day, NULL);
		g_object_get(evt, "starthour", &start_hour, NULL);
		g_object_get(evt, "startmin", &start_min, NULL);
		g_object_get(evt, "endyear", &end_year, NULL);
		g_object_get(evt, "endmonth", &end_month, NULL);
		g_object_get(evt, "endday", &end_day, NULL);
		g_object_get(evt, "endhour", &end_hour, NULL);
		g_object_get(evt, "endmin", &end_min, NULL);
		g_object_get(evt, "isyearly", &is_yearly, NULL);
		g_object_get(evt, "isallday", &is_allday, NULL);
		g_object_get(evt, "ispriority", &is_priority, NULL);
		g_object_get(evt, "hasnotification", &has_notification, NULL);
		g_object_get(evt, "hasreminder", &has_reminder, NULL);
		g_object_get(evt, "remindermin", &reminder_min, NULL);
    
	if(has_notification) {
    
    notify_id =g_strdup_printf("%d", i);
    //create display str   
    
    gchar *time_str = "";
    gchar *starthour_str = "";
    gchar *startmin_str = "";			
    gchar *ampm_str = " ";
    
    if(is_allday)
    {
		time_str="All day event: ";
	}
	else {    
	if (m_12hour_format)
    {
    
    if (start_hour >= 13 && start_hour <= 23)
    {
    int shour = start_hour;
    shour = shour - 12;
    ampm_str = "pm ";
    starthour_str = g_strdup_printf("%d", shour);
    }
    else
    {
    ampm_str = "am ";
    starthour_str = g_strdup_printf("%d", start_hour);
    }
    } // 12
    else
    {
    starthour_str = g_strdup_printf("%d", start_hour);
    } // 24
    
    startmin_str = g_strdup_printf("%d", start_min);
    
    if (start_min < 10)
    {
    time_str = g_strconcat(time_str, starthour_str, ":0", startmin_str, NULL);
    }
    else
    {
    time_str = g_strconcat(time_str, starthour_str, ":", startmin_str, NULL);
    }
    
    time_str = g_strconcat(time_str, ampm_str, NULL);
	
	}//else not allday
	
    display_str = g_strconcat(display_str, time_str, summary_str, "\n", NULL);	
	
	
	NotifyNotification *notify;
	notify_init(notify_id); 
	notify = notify_notification_new("Talk Calendar Notification",display_str , NULL);
	notify_notification_set_urgency (notify, NOTIFY_URGENCY_NORMAL);
	notify_notification_show(notify,NULL);
	
	display_str="";
	
	} //if notification
	} //for i events
}
//======================================================================

void callbk_shutdown(GtkWindow *window, gint response_id, gpointer user_data)
{
	gtk_window_get_default_size(GTK_WINDOW(window), &m_window_width,&m_window_height);
	config_write();	
	
}
//======================================================================
static void startup(GtkApplication *app)
{
	 //STARTUP		  	
	 config_initialize();
	 //g_print("starting database\n");	 	
	 db_create_events_table(); //startup database 
	 //testing
	 
}
//======================================================================
static void activate (GtkApplication* app, gpointer user_data)
{
	GtkWidget *window;	
	GtkWidget *box;
	GtkWidget *calendar; 
	GtkWidget *sw; //scrolled window
	GtkWidget* listbox;
	GtkWidget *label_date; //display date	
	GMenu *menu;
	
	const gchar *home_accels[2] = { "Home", NULL };	
	const gchar *speak_accels[2] = { "space", NULL };
	const gchar *speaktime_accels[2] = {"t", NULL };
	const gchar *newevent_accels[2] = {"<Ctrl>n", NULL };	
	const gchar *editevent_accels[2] = {"<Ctrl>e", NULL };	
	const gchar *info_accels[2] = {"F1", NULL };
	const gchar *delete_accels[2] = {"Delete", NULL };	
	const gchar * preferences_accels[2] = { "<Ctrl><Alt>P", NULL };
	const gchar * quit_accels[2] = { "<Ctrl>Q", NULL };
		
	//setup window
	window = gtk_application_window_new (app);
	gtk_window_set_title (GTK_WINDOW (window), "Talk Calendar");
	gtk_window_set_default_size (GTK_WINDOW (window),m_window_width,m_window_height);
	g_signal_connect (GTK_WINDOW(window), "destroy", G_CALLBACK (callbk_shutdown), NULL);
		
	//setup date label
	label_date = gtk_label_new("");
	gtk_label_set_xalign(GTK_LABEL(label_date), 0.5);
	
	//setup calendar	
	calendar = custom_calendar_new();
	m_start_day = custom_calendar_get_day(CUSTOM_CALENDAR(calendar));
	m_start_month = custom_calendar_get_month(CUSTOM_CALENDAR(calendar));
	m_start_year = custom_calendar_get_year(CUSTOM_CALENDAR(calendar));	
	update_label_date(CUSTOM_CALENDAR(calendar), label_date);
			
	g_signal_connect(CUSTOM_CALENDAR(calendar), "day-selected", G_CALLBACK(callbk_calendar_day_selected), label_date);
	g_signal_connect(CUSTOM_CALENDAR(calendar), "next-month", G_CALLBACK(callbk_calendar_next_month), label_date);
	g_signal_connect(CUSTOM_CALENDAR(calendar), "prev-month", G_CALLBACK(callbk_calendar_prev_month), label_date);
	g_signal_connect(CUSTOM_CALENDAR(calendar), "next-year", G_CALLBACK(callbk_calendar_next_year), label_date);
	g_signal_connect(CUSTOM_CALENDAR(calendar), "prev-year", G_CALLBACK(callbk_calendar_prev_year), label_date);
		
	//setup scrolled window and with listbox inside
	sw = gtk_scrolled_window_new ();
	gtk_widget_set_hexpand (GTK_WIDGET (sw), true);
	gtk_widget_set_vexpand (GTK_WIDGET (sw), true);

	listbox = gtk_list_box_new ();
	gtk_list_box_set_selection_mode (GTK_LIST_BOX (listbox), GTK_SELECTION_SINGLE);
	gtk_list_box_set_show_separators (GTK_LIST_BOX (listbox), TRUE);
	gtk_list_box_set_header_func (GTK_LIST_BOX (listbox), add_separator, NULL, NULL);
		
	 //storage	
    m_store = g_list_store_new (display_item_get_type()); //setup display store
	gtk_list_box_bind_model (GTK_LIST_BOX (listbox), G_LIST_MODEL (m_store), create_widget, NULL, NULL);
	g_signal_connect (listbox, "row-activated", G_CALLBACK (callbk_row_activated),NULL);
	gtk_scrolled_window_set_child (GTK_SCROLLED_WINDOW (sw), listbox);

	//selection
	m_row_index=-1;
    m_id_selection=-1;
   	
	//File actions
	GSimpleAction *export_action;
	export_action=g_simple_action_new("export",NULL); //app.export
	g_action_map_add_action(G_ACTION_MAP(app), G_ACTION(export_action)); //make visible
	g_signal_connect(export_action, "activate",  G_CALLBACK(callbk_export), window);
	
	GSimpleAction *import_action;
	import_action=g_simple_action_new("import",NULL); //app.import
	g_action_map_add_action(G_ACTION_MAP(app), G_ACTION(import_action)); //make visible
	g_signal_connect(import_action, "activate",  G_CALLBACK(callbk_import), window);
	
	GSimpleAction *quit_action;	
	quit_action=g_simple_action_new("quit",NULL); //app.quit
	g_action_map_add_action(G_ACTION_MAP(app), G_ACTION(quit_action)); //make visible	
	g_signal_connect(quit_action, "activate",  G_CALLBACK(callbk_quit), app);
	
	//Edit Actions	
	GSimpleAction *preferences_action;
	preferences_action=g_simple_action_new("preferences",NULL); //app.preferences
	g_action_map_add_action(G_ACTION_MAP(app), G_ACTION(preferences_action)); //make visible
	g_signal_connect(preferences_action, "activate",  G_CALLBACK(callbk_preferences), window);
	
	//Event Actions
	//New event
	GSimpleAction *newevent_action;	
	newevent_action=g_simple_action_new("newevent",NULL); //app.newevent
	g_action_map_add_action(G_ACTION_MAP(app), G_ACTION(newevent_action)); //make visible	
	g_signal_connect(newevent_action, "activate",  G_CALLBACK(callbk_new_event), window);
	
	//Edit Event
	GSimpleAction *editevent_action;	
	editevent_action=g_simple_action_new("editevent",NULL); //app.editevent
	g_action_map_add_action(G_ACTION_MAP(app), G_ACTION(editevent_action)); //make visible	
	g_signal_connect(editevent_action, "activate",  G_CALLBACK(callbk_edit_event), window);	
	//Delete Actions
	GSimpleAction *deleteevent_action;	
	deleteevent_action=g_simple_action_new("deleteevent",NULL); //app.deleteevent
	g_action_map_add_action(G_ACTION_MAP(app), G_ACTION(deleteevent_action)); //make visible	
	g_signal_connect(deleteevent_action, "activate",  G_CALLBACK(callbk_delete_selected), window);
	
	GSimpleAction *deleteall_action;
	deleteall_action=g_simple_action_new("deleteall",NULL); //app.deleteall
	g_action_map_add_action(G_ACTION_MAP(app), G_ACTION(deleteall_action)); //make visible
	g_signal_connect(deleteall_action, "activate",  G_CALLBACK(callbk_delete_all), window);
			
	GSimpleAction *speak_action;	
	speak_action=g_simple_action_new("speak",NULL); //app.speak
	g_action_map_add_action(G_ACTION_MAP(app), G_ACTION(speak_action)); //make visible	
	g_signal_connect(speak_action, "activate",  G_CALLBACK(callbk_speak), window);
	
	GSimpleAction *speaktime_action;	
	speaktime_action=g_simple_action_new("speaktime",NULL); //app.speaktime
	g_action_map_add_action(G_ACTION_MAP(app), G_ACTION(speaktime_action)); //make visible	
	g_signal_connect(speaktime_action, "activate",  G_CALLBACK(callbk_calendar_speaktime), window);
			
	//Calendar Actions	
	GSimpleAction *home_action;	
	home_action=g_simple_action_new("home",NULL); //app.home
	g_action_map_add_action(G_ACTION_MAP(app), G_ACTION(home_action)); //make visible	
	g_signal_connect(home_action, "activate",  G_CALLBACK(callbk_calendar_home), window);
	
	GSimpleAction *colour_action;	
	colour_action=g_simple_action_new("colour",NULL); //app.colour
	g_action_map_add_action(G_ACTION_MAP(app), G_ACTION(colour_action)); //make visible	
	g_signal_connect(colour_action, "activate",  G_CALLBACK(callbk_calendar_colour), window);
				
	//Help Actions
	GSimpleAction *info_action;
	info_action=g_simple_action_new("info",NULL); //app.info
	g_action_map_add_action(G_ACTION_MAP(app), G_ACTION(info_action)); //make visible
	g_signal_connect(info_action, "activate",  G_CALLBACK(callbk_info), window);
	
	GSimpleAction *about_action;
	about_action=g_simple_action_new("about",NULL); //app.about
	g_action_map_add_action(G_ACTION_MAP(app), G_ACTION(about_action)); //make visible
	g_signal_connect(about_action, "activate",  G_CALLBACK(callbk_about), window);
	
	//create menu		
	menu=create_menu(app);	
	gtk_application_set_menubar (app,G_MENU_MODEL(menu));
    gtk_application_window_set_show_menubar(GTK_APPLICATION_WINDOW(window), TRUE);
	
	// connect keyboard accelerators	
	gtk_application_set_accels_for_action(GTK_APPLICATION(app),"app.home", home_accels);
	gtk_application_set_accels_for_action(GTK_APPLICATION(app),"app.speak", speak_accels);	
	gtk_application_set_accels_for_action(GTK_APPLICATION(app),"app.speaktime", speaktime_accels);
	gtk_application_set_accels_for_action(GTK_APPLICATION(app),"app.newevent", newevent_accels);
	gtk_application_set_accels_for_action(GTK_APPLICATION(app),"app.editevent", editevent_accels);
	gtk_application_set_accels_for_action(GTK_APPLICATION(app),"app.deleteevent", delete_accels);	
	gtk_application_set_accels_for_action(GTK_APPLICATION(app),"app.info", info_accels);
	gtk_application_set_accels_for_action(GTK_APPLICATION(app),"app.preferences", preferences_accels);
	gtk_application_set_accels_for_action(GTK_APPLICATION(app),"app.quit", quit_accels);
		
	//setup key-value pairs
	g_object_set_data(G_OBJECT(window), "window-calendar-key",calendar);
	g_object_set_data(G_OBJECT(window), "window-label-date-key",label_date);
	
	//setup layout box
	box =gtk_box_new(GTK_ORIENTATION_VERTICAL,1);	
    gtk_window_set_child (GTK_WINDOW (window), box);
    
    gtk_box_append(GTK_BOX(box), label_date);
	gtk_box_append(GTK_BOX(box), calendar);
	gtk_box_append(GTK_BOX(box), sw); //listbox inside sw
		
	g_object_set(calendar, "todaycolour", m_todaycolour, NULL);
	g_object_set(calendar, "eventcolour", m_eventcolour, NULL);
	g_object_set(calendar, "holidaycolour", m_holidaycolour, NULL);	
	
	custom_calendar_update(CUSTOM_CALENDAR(calendar));
	custom_calendar_goto_today(CUSTOM_CALENDAR(calendar));
	custom_calendar_update(CUSTOM_CALENDAR(calendar));
	
	//start it all up
	if(m_holidays) set_holidays_on_calendar(CUSTOM_CALENDAR(calendar));
	update_label_date(CUSTOM_CALENDAR(calendar), label_date);	
		
	GArray *evt_arry_month; //add month marks
	evt_arry_month = g_array_new(FALSE, FALSE, sizeof(CALENDAR_TYPE_EVENT));
	db_get_all_events_year_month(evt_arry_month, m_start_year,m_start_month);
	//print_array(evt_arry_month);	
	set_marks_on_calendar(CUSTOM_CALENDAR(calendar),evt_arry_month);
	g_array_free(evt_arry_month, FALSE); //clear the array
	
	custom_calendar_update(CUSTOM_CALENDAR(calendar));		
	custom_calendar_goto_today(CUSTOM_CALENDAR(calendar));
	
	GArray *evt_arry_day; //day events	
	evt_arry_day = g_array_new(FALSE, FALSE, sizeof(CALENDAR_TYPE_EVENT)); // setup arraylist
	db_get_all_events_year_month_day(evt_arry_day, m_start_year,m_start_month, m_start_day);
	display_event_array(evt_arry_day);	
	show_notifications(evt_arry_day);	
	g_array_free(evt_arry_day, FALSE); //clear the array 
	
	custom_calendar_update(CUSTOM_CALENDAR(calendar));
			
	custom_calendar_update(CUSTOM_CALENDAR(calendar));
	gtk_window_present (GTK_WINDOW (window));    //use present not show with gtk4
	
	if(m_talk && m_talk_at_startup) {
		speak_events();		
	}			
}
//======================================================================
int main (int argc, char **argv)
{
      
  GtkApplication *app;
  int status;
  app = gtk_application_new ("org.gtk.talkcalendar", G_APPLICATION_DEFAULT_FLAGS);
  g_signal_connect_swapped(app, "startup", G_CALLBACK (startup),app);
  g_signal_connect (app, "activate", G_CALLBACK (activate), NULL);
  status = g_application_run (G_APPLICATION (app), argc, argv);
  g_object_unref (app);

  return status;
}
//======================================================================
