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
#include "diphone.h"
#include "dictionary.h"

#define CONFIG_DIRNAME "talkcal-gtk4-022"
#define CONFIG_FILENAME "talkcal-config-022"

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
static void callbk_check_button_allday_toggled (GtkCheckButton *check_button, gpointer user_data);
static void callbk_add_event(GtkButton *button, gpointer user_data);
static void callbk_new_event(GSimpleAction *action, GVariant *parameter,  gpointer user_data);
static void callbk_update_event(GtkButton *button, gpointer user_data);
static void callbk_edit_event(GSimpleAction *action, GVariant *parameter,  gpointer user_data);
static void callbk_delete_selected(GSimpleAction *action, GVariant *parameter,  gpointer user_data);
static void callbk_delete_all(GSimpleAction *action, GVariant *parameter,  gpointer user_data);
static void callbk_speak(GSimpleAction* action, GVariant *parameter,gpointer user_data);

//calendar menu items
static void callbk_calendar_home(GSimpleAction * action, GVariant *parameter, gpointer user_data);
static void callbk_update_colours(GtkButton *button, gpointer user_data);
static void callbk_calendar_colour(GSimpleAction * action, GVariant *parameter, gpointer user_data);
static void callbk_calendar_speaktime(GSimpleAction * action, GVariant *parameter, gpointer user_data);

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

//Help Callbks
static void callbk_info(GSimpleAction *action, GVariant *parameter,  gpointer user_data);
static void callbk_about(GSimpleAction* action, GVariant *parameter, gpointer user_data);

//listbox display
static GtkWidget *create_widget (gpointer item, gpointer user_data);
static void add_separator (GtkListBoxRow *row, GtkListBoxRow *before, gpointer data);
static void callbk_row_activated (GtkListBox  *listbox,	 GtkListBoxRow *row, gpointer user_data);
static void display_event_array(GArray *evt_arry);

//Speaking
static void speak_events();
static void callbk_speak(GSimpleAction* action, GVariant *parameter,gpointer user_data);

unsigned char *rawcat(unsigned char *arrys[], unsigned int arry_size[], int arry_count);
unsigned int get_merge_size(unsigned int sizes_arry[], int arry_size);
static gpointer thread_playraw(gpointer user_data);


GList* convert_date_to_weekday_diphone_list(int day, int month, int year);
GList* convert_day_number_to_diphone_list(int day_number);
GList* convert_month_to_diphone_list(int month);
GList* get_event_number_diphone_list(int event_number);
GList* convert_number_to_diphone_list(int number);
GList* get_upcoming_number_diphone_list(int number);
static void speak_time(gint hour, gint min);


static void show_notifications(GArray *evt_arry);
GArray*  get_upcoming_array(int upcoming_days);

//config
static char * m_config_file = NULL;

//public holidays UK
gboolean is_public_holiday(int day);
char* get_holiday_str(int day);
GDate* calculate_easter(gint year);

//int get_number_of_events();
int  get_total_number_of_events();
int  get_number_of_day_events();

//removers
static char *remove_commas(const char *text);
static char *remove_fullstop(const char *text);
static char* remove_semicolons (const char *text);
static char* remove_question_marks (const char *text);
static char* remove_explanation_marks (const char *text);
static char* remove_punctuations(const char *text);
static char* replace_hypens(const char *text);
static char* replace_newlines(const char *text);

static GListStore *m_store=NULL;   //m_store is a Gio GListStore store (not GktListStore which is being depreciated)
static int m_id_selection=-1;
static int m_row_index=-1; //listbox row index
static int m_store_size=0;

//calendar
static int m_12hour_format=1; //am pm hour format
static int m_show_end_time=0; //show end_time
static int m_show_location=1; //show location
static int m_holidays=1; //holidays

static int m_talk =1;
static int m_talk_at_startup =0;
static int m_talk_event_number=1;
static int m_talk_time=1;
static int m_talk_location =1;

static int m_talk_rate=16000;

static int m_talk_upcoming=0; 
static int m_talk_overlap=0; //to do
static int m_talk_priority=0; //to do

static int m_reset_preferences=0;

static int m_today_year=0;
static int m_today_month=0;
static int m_today_day=0;

static int m_window_width=500;
static int m_window_height=400;

static const char* m_summary ="summary";
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

static const char* m_todaycolour="lightblue";
static const char* m_eventcolour="lightpink";
static const char* m_holidaycolour="lightseagreen";
static gboolean m_frame=FALSE;

CalendarEvent *selected_evt; 


const GActionEntry app_actions[] = {
  { "speak", callbk_speak },  
  { "home", callbk_calendar_home},
  { "newevent", callbk_new_event},
  { "editevent", callbk_edit_event},
  { "speaktime", callbk_calendar_speaktime},
  { "deleteevent", callbk_delete_selected},
  { "info", callbk_info},
  { "preferences", callbk_preferences},
  { "quit", callbk_quit}
};
//-------------------------------------------------------------------------------
// Save load config file
//-------------------------------------------------------------------------------

static void config_load_default()
{
	//talking	
	m_talk=1;
	m_talk_at_startup=1;
	m_talk_event_number=1;
	m_talk_time=1;
	m_talk_location=1;		
	m_talk_overlap=0; //to do
	m_talk_priority=0; 
	m_talk_upcoming=0;
	m_talk_rate=16000;	
	//calendar
	m_12hour_format=1;
	m_show_end_time=0;
	m_show_location=1;
	m_holidays=1;
	m_window_width=500;
	m_window_height=400;
	m_todaycolour="lightblue";
	m_eventcolour="lightpink";
	m_holidaycolour="lightseagreen";
	m_frame=FALSE;
}

static void config_read()
{
	// Clean up previously loaded configuration values	
	m_talk=1;
	m_talk_at_startup=1;
	m_talk_event_number=1;
	m_talk_time=1;
	m_talk_location=1;
	m_talk_overlap=0; //to do
	m_talk_priority=0;
	m_talk_upcoming=0; 
	m_talk_rate=16000;	
	m_12hour_format=1;
	m_show_end_time=0;
	m_show_location=1;
	m_holidays=1;
	m_window_width=500;
	m_window_height=400;
	m_todaycolour="lightblue";
	m_eventcolour="lightpink";
    m_holidaycolour="lightseagreen";
    m_frame=FALSE;
	// Load keys from keyfile
	GKeyFile * kf = g_key_file_new();
	g_key_file_load_from_file(kf, m_config_file, G_KEY_FILE_NONE, NULL);

	//talk	
	m_talk = g_key_file_get_integer(kf, "calendar_settings", "talk", NULL);
	m_talk_at_startup=g_key_file_get_integer(kf, "calendar_settings", "talk_startup", NULL);
	m_talk_event_number=g_key_file_get_integer(kf, "calendar_settings", "talk_event_number", NULL);
	m_talk_time=g_key_file_get_integer(kf, "calendar_settings", "talk_time", NULL);
	m_talk_location=g_key_file_get_integer(kf, "calendar_settings", "talk_location", NULL);
	m_talk_overlap=g_key_file_get_integer(kf, "calendar_settings", "talk_overlap", NULL);
	m_talk_priority=g_key_file_get_integer(kf, "calendar_settings", "talk_priority", NULL);
	m_talk_upcoming=g_key_file_get_integer(kf, "calendar_settings", "talk_upcoming", NULL);

	m_talk_rate=g_key_file_get_integer(kf, "calendar_settings", "sample_rate", NULL);
	
	m_12hour_format=g_key_file_get_integer(kf, "calendar_settings", "hour_format", NULL);
	m_show_end_time = g_key_file_get_integer(kf, "calendar_settings", "show_end_time", NULL);
	m_show_location = g_key_file_get_integer(kf, "calendar_settings", "show_location", NULL);

	m_holidays = g_key_file_get_integer(kf, "calendar_settings", "holidays", NULL);
	
	m_window_width = g_key_file_get_integer(kf, "calendar_settings", "window_width", NULL);
	m_window_height=g_key_file_get_integer(kf, "calendar_settings", "window_height", NULL);
	
	m_todaycolour=g_key_file_get_string(kf, "calendar_settings", "todaycolour", NULL);
    m_eventcolour=g_key_file_get_string(kf, "calendar_settings", "eventcolour", NULL);
    m_holidaycolour=g_key_file_get_string(kf, "calendar_settings", "holidaycolour", NULL);
    
    m_frame=g_key_file_get_boolean(kf, "calendar_settings", "frame", NULL);
	
	g_key_file_free(kf);
}

void config_write()
{

	GKeyFile * kf = g_key_file_new();

	//talk
	
	g_key_file_set_integer(kf, "calendar_settings", "talk", m_talk);
	g_key_file_set_integer(kf, "calendar_settings", "talk_startup", m_talk_at_startup);
	g_key_file_set_integer(kf, "calendar_settings", "talk_event_number", m_talk_event_number);
	g_key_file_set_integer(kf, "calendar_settings", "talk_time", m_talk_time);
	g_key_file_set_integer(kf, "calendar_settings", "talk_location", m_talk_location);
	g_key_file_set_integer(kf, "calendar_settings", "talk_overlap", m_talk_overlap);
	g_key_file_set_integer(kf, "calendar_settings", "talk_priority", m_talk_priority);
	g_key_file_set_integer(kf, "calendar_settings", "talk_upcoming", m_talk_upcoming);

	g_key_file_set_integer(kf, "calendar_settings", "sample_rate", m_talk_rate);

	
	g_key_file_set_integer(kf, "calendar_settings", "hour_format", m_12hour_format);
	g_key_file_set_integer(kf, "calendar_settings", "show_end_time", m_show_end_time);
	g_key_file_set_integer(kf, "calendar_settings", "show_location", m_show_location);

	g_key_file_set_integer(kf, "calendar_settings", "holidays", m_holidays);
	
	g_key_file_set_integer(kf, "calendar_settings", "window_width", m_window_width);
	g_key_file_set_integer(kf, "calendar_settings", "window_height", m_window_height);
	g_key_file_set_string(kf, "calendar_settings", "todaycolour", m_todaycolour);
	g_key_file_set_string(kf, "calendar_settings", "eventcolour", m_eventcolour);
	g_key_file_set_string(kf, "calendar_settings", "holidaycolour", m_holidaycolour);
	
	g_key_file_set_boolean(kf, "calendar_settings", "frame", m_frame);

	gsize length;
	gchar * data = g_key_file_to_data(kf, &length, NULL);
	g_file_set_contents(m_config_file, data, -1, NULL);
	g_free(data);
	g_key_file_free(kf);

}

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

//====================================================================
// Removers (unwanted characters}
//--------------------------------------------------------------------

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
		{ 
			g_string_append_unichar(str, *p);
		} // if
		++p;
	}
	return g_string_free(str, FALSE);
}

static char *remove_fullstop(const char *text)
{
	GString *str;
	const char *p;
	str = g_string_new("");
	p = text;
	while (*p)
	{
		gunichar cp = g_utf8_get_char(p);
		if (cp != '.')
		{ 
			g_string_append_unichar(str, *p);
		} // if
		++p;
	}
	return g_string_free(str, FALSE);
}
static char* remove_semicolons (const char *text)
{
	GString *str;
	const char *p;
	str = g_string_new ("");
	p = text;
	while (*p)
	{
	gunichar cp = g_utf8_get_char(p);
	if ( cp != ';' ){ 
	g_string_append_unichar (str, *p);
	}//if
	++p;
	}
	return g_string_free (str, FALSE);
}

static char* remove_question_marks (const char *text)
{
	GString *str;
	const char *p;
	str = g_string_new ("");
	p = text;
	while (*p)
	{
	gunichar cp = g_utf8_get_char(p);
	if ( cp != '?' ){ 
	g_string_append_unichar (str, *p);
	}//if
	++p;
	}
	return g_string_free (str, FALSE);
}

static char* remove_explanation_marks (const char *text)
{
	GString *str;
	const char *p;
	str = g_string_new ("");
	p = text;
	while (*p)
	{
	gunichar cp = g_utf8_get_char(p);
	if ( cp != '!' ){ 
	g_string_append_unichar (str, *p);
	}//if
	++p;
	}
	return g_string_free (str, FALSE);
}

static char* remove_punctuations(const char *text)
{
	GString *str;
	const char *p;
	str = g_string_new ("");
	p = text;
	while (*p)
	{
	gunichar cp = g_utf8_get_char(p);
	if ( cp != '\'' ){ //remove all apostrophes
	g_string_append_unichar (str, *p);
	}//if
	++p;
	}
	return g_string_free (str, FALSE);
}

static char* replace_hypens(const char *text)
{
	GString *str;
	const char *p;
	str = g_string_new ("");
	p = text;
	gint i=0;
	while (*p)
	{
	gunichar cp = g_utf8_get_char(p);
	if ( cp != '-' ){ //append
	g_string_append_unichar (str, *p);
	}//if
	if ( cp == '-' ){ //replace			
	g_string_insert_unichar (str,i,' ');
	}//if	
	++p;
	++i;
	}
	return g_string_free (str, FALSE);
}

static char* replace_newlines(const char *text)
{
	GString *str;
	const char *p;
	str = g_string_new ("");
	p = text;
	gint i=0;
	while (*p)
	{
	gunichar cp = g_utf8_get_char(p);
	if ( cp != '\n' ){ //append
	g_string_append_unichar (str, *p);
	}//if
	if ( cp == '\n' ){ //replace			
	g_string_insert_unichar (str,i,' ');
	}//if	
	++p;
	++i;
	}
	return g_string_free (str, FALSE);
}



//====================================================================
int  get_total_number_of_events(){

	return db_get_number_of_rows_all();
}

//----------------------------------------------------------------------
// Number of day events
//----------------------------------------------------------------------

int  get_number_of_day_events(){
	int event_count=db_get_number_of_rows_year_month_day(m_start_year, m_start_month, m_start_day);	
	return event_count;
}


//---------------------------------------------------------------------
// Public holidays
//----------------------------------------------------------------------

//---------------------------------------------------------------------
// calculate easter
//---------------------------------------------------------------------

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
//--------------------------------------------------------------------
// public holidays
//---------------------------------------------------------------------
gboolean is_public_holiday(int day) {

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

char* get_holiday_str(int day) {

// UK public holidays
// New Year's Day: 1 January (DONE)
// Good Friday: March or April  (DONE)
// Easter Monday: March or April (DONE)
// Early May: First Monday of May (TODO)
// Spring Bank Holiday: Last Monday of May (DONE)
// Summer Bank Holiday: Last Monday of August (DONE)
// Christmas Day: 25 December (DONE)
// Boxing day: 26 December (DONE)

	//markup public holidays
	if (m_start_month==1 && day ==1) {
	return " New Year ";
	}

	if (m_start_month==12 && day==25) {
	//christmas day
	return " Christmas Day ";
	}

	if (m_start_month==12 && day==26) {
	//boxing day
	return " Boxing Day ";
	}

	if (m_start_month == 5) {
     //May complicated
     GDate *first_monday_may;
     first_monday_may = g_date_new_dmy(1, m_start_month, m_start_year);


     while (g_date_get_weekday(first_monday_may) != G_DATE_MONDAY)
       g_date_add_days(first_monday_may,1);

     int may_day = g_date_get_day(first_monday_may);

     if( day==may_day) return "May Bank Holiday "; //may bank holiday

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
     return " Spring Bank Holiday ";   //spring bank holiday

	} //m_start_month ==5 (May)

	GDate *easter_date =calculate_easter(m_start_year);
	int easter_day = g_date_get_day(easter_date);
	int easter_month =g_date_get_month(easter_date);

	if(m_start_month==easter_month && day == easter_day)
	{
	//easter day
	return " Easter Day ";
	}

	g_date_subtract_days(easter_date,2);
	int easter_friday = g_date_get_day(easter_date);
	int easter_friday_month =g_date_get_month(easter_date);

	if(m_start_month==easter_friday_month && day ==easter_friday)
	{
	//easter friday
	return " Easter Friday ";
	}

	g_date_add_days(easter_date,3);
	int easter_monday = g_date_get_day(easter_date); //easter monday
	int easter_monday_month =g_date_get_month(easter_date);

	if(m_start_month==easter_monday_month && day ==easter_monday)
	{
	//easter monday
	return " Easter Monday ";
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
     return " August Bank Holiday ";   //august bank holiday

    } //m_start_month==8

	return "";
}

//----------------------------------------------------------------------
//Implementation of file menu callbks
//----------------------------------------------------------------------

static void callbk_export(GSimpleAction *action, GVariant *parameter,  gpointer user_data)
{
	//g_print("export csv called\n");
	export_csv_file();
}

static void callbk_import(GSimpleAction *action, GVariant *parameter,  gpointer user_data)
{
	//g_print("import csv called\n");
	import_csv_file(user_data);
}

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


void import_csv_file(gpointer user_data) {
	
	GtkWidget *window = user_data;
	GtkWidget *calendar = g_object_get_data(G_OBJECT(window), "window-calendar-key");
	//GtkWidget *label_date = g_object_get_data(G_OBJECT(window), "window-label-date-key");

	
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
	//print_array(evt_arry_month);
	set_marks_on_calendar(CUSTOM_CALENDAR(calendar),evt_arry_month);
	g_array_free(evt_arry_month, FALSE); //clear the array
	custom_calendar_update(CUSTOM_CALENDAR(calendar));
	g_object_unref(file_stream);
	g_object_unref(file);	
}


//---------------------------------------------------------------------

static void callbk_quit(GSimpleAction * action,	G_GNUC_UNUSED GVariant *parameter, gpointer user_data)
{
	//g_print("quit callbk\n");	
	g_application_quit(G_APPLICATION(user_data));		
}

//----------------------------------------------------------------------
//Implementation of Edit menu callbks
//----------------------------------------------------------------------
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
	GtkWidget *check_button_frame= g_object_get_data(G_OBJECT(button), "check-button-frame-key");

	//talking
	GtkWidget *check_button_talk= g_object_get_data(G_OBJECT(button), "check-button-talk-key");
    GtkWidget *check_button_talk_startup= g_object_get_data(G_OBJECT(button), "check-button-talk-startup-key");
    GtkWidget *check_button_talk_event_number= g_object_get_data(G_OBJECT(button), "check-button-talk-event-number-key");

	GtkWidget *check_button_talk_time= g_object_get_data(G_OBJECT(button), "check-button-talk-time-key");
	GtkWidget *check_button_talk_location=g_object_get_data(G_OBJECT(button), "check-button-talk-location-key");

	GtkWidget *check_button_talk_priority= g_object_get_data(G_OBJECT(button), "check-button-talk-priority-key");
	GtkWidget *check_button_talk_upcoming= g_object_get_data(G_OBJECT(button), "check-button-talk-upcoming-key");
	
	GtkWidget *spin_button_talk_rate = g_object_get_data(G_OBJECT(button), "spin-talk-rate-key");
	
    GtkWidget *check_button_reset_all= g_object_get_data(G_OBJECT(button), "check-button-reset-all-key");

	m_12hour_format=gtk_check_button_get_active (GTK_CHECK_BUTTON(check_button_hour_format));
	m_show_end_time=gtk_check_button_get_active (GTK_CHECK_BUTTON(check_button_show_end_time));
	m_show_location=gtk_check_button_get_active (GTK_CHECK_BUTTON(check_button_show_location));

	m_holidays=gtk_check_button_get_active (GTK_CHECK_BUTTON(check_button_holidays));
	m_frame=gtk_check_button_get_active (GTK_CHECK_BUTTON(check_button_frame));
	
	//g_print("Preferences: m_frame = %d\n",m_frame);

	m_talk=gtk_check_button_get_active (GTK_CHECK_BUTTON(check_button_talk));
	m_talk_at_startup=gtk_check_button_get_active (GTK_CHECK_BUTTON(check_button_talk_startup));
	m_talk_event_number=gtk_check_button_get_active (GTK_CHECK_BUTTON(check_button_talk_event_number));

	m_talk_time =gtk_check_button_get_active (GTK_CHECK_BUTTON(check_button_talk_time));
	m_talk_location =gtk_check_button_get_active (GTK_CHECK_BUTTON(check_button_talk_location));
	m_talk_priority =gtk_check_button_get_active (GTK_CHECK_BUTTON(check_button_talk_priority));
	m_talk_upcoming =gtk_check_button_get_active (GTK_CHECK_BUTTON(check_button_talk_upcoming));

	m_talk_rate = gtk_spin_button_get_value(GTK_SPIN_BUTTON(spin_button_talk_rate));

	
	m_reset_preferences=gtk_check_button_get_active(GTK_CHECK_BUTTON(check_button_reset_all));

	if(m_reset_preferences) {
	//reset everything
	m_12hour_format=1;
	m_show_end_time=0;
	m_show_location=1;
	m_holidays=0;
	m_frame=0;
	m_talk_rate=16000;
	m_talk = 1;
	m_talk_at_startup=1;
	m_talk_event_number=1;
	m_talk_time=1;
	m_talk_location=0;	
	m_talk_priority=0;
	m_talk_upcoming=0;
	m_talk_overlap=0;	
    m_reset_preferences=0; //toggle
	}

	//save preferences
	config_write();	
	g_object_set(calendar, "frame", m_frame, NULL);	
	custom_calendar_update(CUSTOM_CALENDAR(calendar));
	update_label_date(CUSTOM_CALENDAR(calendar), label_date);
		
	GArray *evt_arry_day; //normal day events	
	evt_arry_day = g_array_new(FALSE, FALSE, sizeof(CALENDAR_TYPE_EVENT)); // setup arraylist
	db_get_all_events_year_month_day(evt_arry_day, m_start_year,m_start_month, m_start_day);
	//print_array(evt_arry_day);
	//g_list_store_remove_all(m_store); // clear	
	display_event_array(evt_arry_day);
	g_array_free(evt_arry_day, FALSE); //clear the array 
	gtk_window_destroy(GTK_WINDOW(dialog));
}

static void callbk_preferences(GSimpleAction* action, GVariant *parameter,gpointer user_data)
{
	//g_print("Preferences callbk\n");
	GtkWidget *window =user_data;

	GtkWidget *dialog;
	GtkWidget *box;
	//Check buttons
	GtkWidget *check_button_talk;
	GtkWidget *check_button_talk_startup;
	GtkWidget *check_button_talk_event_number;
	GtkWidget *check_button_talk_time;
	GtkWidget *check_button_talk_location;
	GtkWidget *check_button_talk_priority;
	GtkWidget *check_button_talk_upcoming;
	GtkWidget *check_button_hour_format;
	GtkWidget *check_button_show_end_time;
	GtkWidget *check_button_show_location;
	GtkWidget *check_button_holidays;
	GtkWidget *check_button_frame;
	GtkWidget *check_button_reset_all;

	GtkWidget *button_set;

	GtkWidget *label_talk_rate;
	GtkWidget *spin_button_talk_rate;
	GtkWidget *box_talk_rate;
			

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

	check_button_holidays = gtk_check_button_new_with_label ("Public Holidays");
	check_button_frame = gtk_check_button_new_with_label ("Show Calendar Grid");

	//talk
	check_button_talk = gtk_check_button_new_with_label ("Talk");
	check_button_talk_startup = gtk_check_button_new_with_label ("Talk At Startup");
	check_button_talk_event_number = gtk_check_button_new_with_label ("Talk Event Number");

	check_button_talk_time= gtk_check_button_new_with_label ("Talk Time");
	check_button_talk_location= gtk_check_button_new_with_label ("Talk Location");
	check_button_talk_priority= gtk_check_button_new_with_label ("Talk Priority");
	check_button_talk_upcoming= gtk_check_button_new_with_label ("Talk Upcoming");

	check_button_reset_all = gtk_check_button_new_with_label ("Reset All");

	//sample rate
	GtkAdjustment *adjustment_talk_rate;
	// value,lower,upper,step_increment,page_increment,page_size
	adjustment_talk_rate = gtk_adjustment_new(16000.00, 8000.00, 24000.00, 1000.0, 1000.0, 0.0);
	// start time spin
	label_talk_rate = gtk_label_new("Talk Rate ");
	spin_button_talk_rate = gtk_spin_button_new(adjustment_talk_rate, 1000, 0);
	gtk_spin_button_set_value(GTK_SPIN_BUTTON(spin_button_talk_rate), m_talk_rate);
	box_talk_rate = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 1);
	gtk_box_append(GTK_BOX(box_talk_rate), label_talk_rate);
	gtk_box_append(GTK_BOX(box_talk_rate), spin_button_talk_rate);
	

  
	//calendar
	gtk_check_button_set_active (GTK_CHECK_BUTTON(check_button_hour_format),m_12hour_format);
	gtk_check_button_set_active (GTK_CHECK_BUTTON(check_button_show_end_time), m_show_end_time);
	gtk_check_button_set_active (GTK_CHECK_BUTTON(check_button_show_location), m_show_location);

	gtk_check_button_set_active (GTK_CHECK_BUTTON(check_button_holidays),m_holidays);
	gtk_check_button_set_active (GTK_CHECK_BUTTON(check_button_frame),m_frame);

	//talk
	gtk_check_button_set_active (GTK_CHECK_BUTTON(check_button_talk), m_talk);
	gtk_check_button_set_active (GTK_CHECK_BUTTON(check_button_talk_startup), m_talk_at_startup);
	gtk_check_button_set_active (GTK_CHECK_BUTTON(check_button_talk_event_number), m_talk_event_number);

	gtk_check_button_set_active (GTK_CHECK_BUTTON(check_button_talk_time), m_talk_time);
	gtk_check_button_set_active (GTK_CHECK_BUTTON(check_button_talk_location), m_talk_location);
	gtk_check_button_set_active (GTK_CHECK_BUTTON(check_button_talk_priority), m_talk_priority);
	gtk_check_button_set_active (GTK_CHECK_BUTTON(check_button_talk_upcoming), m_talk_upcoming);

	gtk_check_button_set_active (GTK_CHECK_BUTTON(check_button_reset_all), m_reset_preferences);

	//data setters
	g_object_set_data(G_OBJECT(button_set), "dialog-key",dialog);
	//calendar
	g_object_set_data(G_OBJECT(button_set), "check-button-hour-format-key",check_button_hour_format);
	g_object_set_data(G_OBJECT(button_set), "check-button-show-end-time-key",check_button_show_end_time);
	g_object_set_data(G_OBJECT(button_set), "check-button-show-location-key",check_button_show_location);

	g_object_set_data(G_OBJECT(button_set), "check-button-holidays-key",check_button_holidays);
	g_object_set_data(G_OBJECT(button_set), "check-button-frame-key",check_button_frame);

	g_object_set_data(G_OBJECT(button_set), "check-button-talk-key",check_button_talk);
	g_object_set_data(G_OBJECT(button_set), "check-button-talk-startup-key",check_button_talk_startup);
	g_object_set_data(G_OBJECT(button_set), "check-button-talk-event-number-key",check_button_talk_event_number);

	g_object_set_data(G_OBJECT(button_set), "check-button-talk-time-key",check_button_talk_time);
	g_object_set_data(G_OBJECT(button_set), "check-button-talk-location-key",check_button_talk_location);
	g_object_set_data(G_OBJECT(button_set), "check-button-talk-priority-key",check_button_talk_priority);
	g_object_set_data(G_OBJECT(button_set), "check-button-talk-upcoming-key",check_button_talk_upcoming);


	g_object_set_data(G_OBJECT(button_set), "check-button-reset-all-key",check_button_reset_all);

	g_object_set_data(G_OBJECT(button_set), "spin-talk-rate-key", spin_button_talk_rate);
	
	
	gtk_box_append(GTK_BOX(box), check_button_hour_format);
	gtk_box_append(GTK_BOX(box), check_button_show_end_time);
	gtk_box_append(GTK_BOX(box), check_button_show_location);
	gtk_box_append(GTK_BOX(box), check_button_holidays);
	gtk_box_append(GTK_BOX(box), check_button_frame);

	gtk_box_append(GTK_BOX(box), check_button_talk);
	gtk_box_append(GTK_BOX(box), check_button_talk_startup);
	gtk_box_append(GTK_BOX(box), check_button_talk_event_number);
	gtk_box_append(GTK_BOX(box), check_button_talk_time);
	gtk_box_append(GTK_BOX(box), check_button_talk_location);
	gtk_box_append(GTK_BOX(box), check_button_talk_priority);
	gtk_box_append(GTK_BOX(box), check_button_talk_upcoming);
		
	gtk_box_append(GTK_BOX(box), box_talk_rate);
	gtk_box_append(GTK_BOX(box), check_button_reset_all);
	gtk_box_append(GTK_BOX(box), button_set);
	gtk_window_present (GTK_WINDOW (dialog));	
	
}


//----------------------------------------------------------------------
//Implementation of event menu callbks
//----------------------------------------------------------------------

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

	buffer_summary = gtk_entry_get_buffer(GTK_ENTRY(entry_summary));
	m_summary = gtk_entry_buffer_get_text(buffer_summary);
	
	char* summary = g_strdup(m_summary);//duplicate as const
	
	m_summary =g_strchomp(summary); //remove trailing whitespace
	m_summary = remove_semicolons(m_summary);
	m_summary = remove_commas(m_summary);
	m_summary = remove_fullstop(m_summary);
	m_summary = remove_question_marks (m_summary);
	m_summary= remove_explanation_marks (m_summary);
	m_summary = remove_punctuations(m_summary); //for database entry
	m_summary = replace_hypens(m_summary);
	m_summary = replace_newlines(m_summary);
	
	buffer_location = gtk_entry_get_buffer(GTK_ENTRY(entry_location));
	m_location = gtk_entry_buffer_get_text(buffer_location);
	
	char* location = g_strdup(m_location);
	
	m_location =g_strchomp(location); //remove trailing spaces
	//gchar* g_strchug (gchar* string) removes leading spaces	
	m_location = remove_semicolons(m_location);
	m_location = remove_commas(m_location);
	m_location =remove_fullstop(m_location);
	m_location =remove_question_marks (m_location);
	m_location =remove_explanation_marks (m_location);
	m_location = remove_punctuations(m_location);	
	m_location = replace_newlines(m_location);
	m_location = replace_hypens(m_location); //replace location hypens
	

	buffer_description = gtk_entry_get_buffer(GTK_ENTRY(entry_description));
	m_description = gtk_entry_buffer_get_text(buffer_description);
	
	float start_time = gtk_spin_button_get_value(GTK_SPIN_BUTTON(spin_button_start_time));
	float end_time = gtk_spin_button_get_value(GTK_SPIN_BUTTON(spin_button_end_time));

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
	//g_print("insert event: isallday = %d\n",isallday);
	g_object_set(evt, "ispriority", priority, NULL);
	g_object_set(evt, "hasnotification", notification, NULL);
	g_object_set(evt, "hasreminder", 0, NULL);
	g_object_set(evt, "remindermin", 30, NULL);

	db_insert_event(evt); //insert event into database
	

	m_id_selection = -1;
	update_label_date(CUSTOM_CALENDAR(calendar), label_date);
	
	custom_calendar_reset_marks(CUSTOM_CALENDAR(calendar));		
	GArray *evt_arry_month; //add month marks
	evt_arry_month = g_array_new(FALSE, FALSE, sizeof(CALENDAR_TYPE_EVENT));
	db_get_all_events_year_month(evt_arry_month, m_start_year,m_start_month);
	//print_array(evt_arry_month);
	set_marks_on_calendar(CUSTOM_CALENDAR(calendar),evt_arry_month);
	g_array_free(evt_arry_month, FALSE); //clear the array
		
	custom_calendar_update(CUSTOM_CALENDAR(calendar));
	
	GArray *evt_arry_day;
	//g_print("year = %d, month = %d day = %d\n",year,month,day);
	evt_arry_day = g_array_new(FALSE, FALSE, sizeof(CALENDAR_TYPE_EVENT)); // setup arraylist
	db_get_all_events_year_month_day(evt_arry_day, m_start_year,m_start_month, m_start_day);
	//print_array(evt_arry_day);	
	display_event_array(evt_arry_day);
	g_array_free(evt_arry_day, FALSE); //clear the array 
	
	
	gtk_window_destroy(GTK_WINDOW(dialog));
	// gtk_window_close(GTK_WINDOW(window));
}

static void callbk_new_event(GSimpleAction *action, GVariant *parameter,  gpointer user_data)
//static void callbk_new_event(GtkButton *button, gpointer  user_data)
{
	//g_print("New event callbk\n");
	GtkWidget *window = user_data;

	GtkWidget *dialog;
	gint response;
	GtkWidget *grid;
	GtkWidget *box;
	GtkWidget *label_date;
	GtkEntryBuffer *buffer;

	GtkWidget *label_entry_summary;
	GtkWidget *entry_summary;

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

	label_entry_summary = gtk_label_new("Event Summary");
	entry_summary = gtk_entry_new();
	gtk_entry_set_max_length(GTK_ENTRY(entry_summary), 150);

	label_location = gtk_label_new("Location");
	entry_location = gtk_entry_new();
	gtk_entry_set_max_length(GTK_ENTRY(entry_location), 100);

	label_description = gtk_label_new("Description");
	entry_description = gtk_entry_new();
	gtk_entry_set_max_length(GTK_ENTRY(entry_description), 200);

	gtk_box_append(GTK_BOX(box), label_date);
	gtk_box_append(GTK_BOX(box), label_entry_summary);
	gtk_box_append(GTK_BOX(box), entry_summary);
	
	gtk_box_append(GTK_BOX(box), label_description);
	gtk_box_append(GTK_BOX(box), entry_description);

	gtk_box_append(GTK_BOX(box), label_location);
	gtk_box_append(GTK_BOX(box), entry_location);


	g_object_set_data(G_OBJECT(button_add), "dialog-key", dialog);
	g_object_set_data(G_OBJECT(button_add), "entry-summary-key", entry_summary);
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
//---------------------------------------------------------------------

static void callbk_update_event(GtkButton *button, gpointer user_data)
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

	GtkWidget *check_button_allday = g_object_get_data(G_OBJECT(button), "check-button-allday-key");
	GtkWidget *check_button_isyearly = g_object_get_data(G_OBJECT(button), "check-button-isyearly-key");
	GtkWidget *check_button_priority = g_object_get_data(G_OBJECT(button), "check-button-priority-key");
	GtkWidget *check_button_notification = g_object_get_data(G_OBJECT(button), "check-button-notification-key");

	
	GtkWidget *spin_button_start_time = g_object_get_data(G_OBJECT(button), "spin-start-time-key");
	GtkWidget *spin_button_end_time = g_object_get_data(G_OBJECT(button), "spin-end-time-key");

	buffer_summary = gtk_entry_get_buffer(GTK_ENTRY(entry_summary));
	m_summary = gtk_entry_buffer_get_text(buffer_summary);
	
	char* summary = g_strdup(m_summary);//duplicate as const
	
	m_summary =g_strchomp(summary); //remove trailing whitespace
	m_summary = remove_semicolons(m_summary);
	m_summary = remove_commas(m_summary);
	m_summary = remove_fullstop(m_summary);
	m_summary = remove_question_marks (m_summary);
	m_summary = remove_explanation_marks (m_summary);
	m_summary = remove_punctuations(m_summary); //need to remove for database
	m_summary = replace_hypens(m_summary);
	m_summary = replace_newlines(m_summary);
	
	buffer_location = gtk_entry_get_buffer(GTK_ENTRY(entry_location));
	m_location = gtk_entry_buffer_get_text(buffer_location);
	
	char * location = g_strdup(m_location);
	m_location =g_strchomp(location); //remove trailing spaces
	//gchar* g_strchug (gchar* string) removes leading spaces	
	m_location = remove_semicolons(m_location);
	m_location = remove_commas(m_location);
	m_location =remove_fullstop(m_location);
	m_location =remove_question_marks (m_location);
	m_location =remove_explanation_marks (m_location);
	m_location = remove_punctuations(m_location);	
	m_location = replace_newlines(m_location);
	m_location = replace_hypens(m_location); //replace location hypens
	
	buffer_description = gtk_entry_get_buffer(GTK_ENTRY(entry_description));
	m_description = gtk_entry_buffer_get_text(buffer_description);
	//m_description = remove_semicolons(m_description);
	//m_description = remove_commas(m_description);

	// m_description="textview todo";
	float start_time = gtk_spin_button_get_value(GTK_SPIN_BUTTON(spin_button_start_time));
	float end_time = gtk_spin_button_get_value(GTK_SPIN_BUTTON(spin_button_end_time));

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
		
	db_update_event(selected_evt);

	
	m_row_index = -1;
	m_id_selection = -1;
	
	update_label_date(CUSTOM_CALENDAR(calendar), label_date);
	
	custom_calendar_reset_marks(CUSTOM_CALENDAR(calendar));		
	GArray *evt_arry_month; //add month marks
	evt_arry_month = g_array_new(FALSE, FALSE, sizeof(CALENDAR_TYPE_EVENT));
	db_get_all_events_year_month(evt_arry_month, m_start_year,m_start_month);
	//print_array(evt_arry_month);
	set_marks_on_calendar(CUSTOM_CALENDAR(calendar),evt_arry_month);
	g_array_free(evt_arry_month, FALSE); //clear the array
		
	custom_calendar_update(CUSTOM_CALENDAR(calendar));
	
	GArray *evt_arry_day;
	//g_print("year = %d, month = %d day = %d\n",year,month,day);
	evt_arry_day = g_array_new(FALSE, FALSE, sizeof(CALENDAR_TYPE_EVENT)); // setup arraylist
	db_get_all_events_year_month_day(evt_arry_day, m_start_year,m_start_month, m_start_day);
	//print_array(evt_arry_day);	
	display_event_array(evt_arry_day);
	g_array_free(evt_arry_day, FALSE); //clear the array 
		
	gtk_window_destroy(GTK_WINDOW(dialog));
	
}

//static void callbk_edit_event(GtkButton *button, gpointer  user_data)
static void callbk_edit_event(GSimpleAction *action, GVariant *parameter,  gpointer user_data)
{
	//g_print("Edit event callbk\n");
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
	GtkWidget *entry_summary;

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
	//g_print("hasnotification = %d\n",has_notification);
	g_object_get (selected_evt, "hasreminder", &has_reminder, NULL);
	g_object_get (selected_evt, "remindermin", &reminder_min, NULL);
	
	m_summary = g_strdup(summary_str);
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

	label_entry_summary = gtk_label_new("Event Summary");
	entry_summary = gtk_entry_new();
	gtk_entry_set_max_length(GTK_ENTRY(entry_summary), 150);
	buffer_summary = gtk_entry_buffer_new(m_summary, -1); // show  event summary
	gtk_entry_set_buffer(GTK_ENTRY(entry_summary), buffer_summary);

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
	gtk_box_append(GTK_BOX(box), entry_summary);
	
	gtk_box_append(GTK_BOX(box), label_description);
	gtk_box_append(GTK_BOX(box), entry_description);

	gtk_box_append(GTK_BOX(box), label_location);
	gtk_box_append(GTK_BOX(box), entry_location);

	g_object_set_data(G_OBJECT(button_update), "dialog-key", dialog);

	g_object_set_data(G_OBJECT(button_update), "entry-summary-key", entry_summary);
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
	// gtk_check_button_set_active (GTK_CHECK_BUTTON(check_button_allday), e.is_allday);

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

//---------------------------------------------------------------------
//static void callbk_delete_selected(GtkButton *button, gpointer  user_data)
static void callbk_delete_selected(GSimpleAction *action, GVariant *parameter,  gpointer user_data)
{
	//g_print("Delete Selected callbk\n");
	
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
	//print_array(evt_arry_month);
	set_marks_on_calendar(CUSTOM_CALENDAR(calendar),evt_arry_month);
	g_array_free(evt_arry_month, FALSE); //clear the array
	
	//update day events	
	g_list_store_remove_all(m_store); // clear
	
	GArray *evt_arry_day;	
	evt_arry_day = g_array_new(FALSE, FALSE, sizeof(CALENDAR_TYPE_EVENT)); // setup arraylist
	db_get_all_events_year_month_day(evt_arry_day, m_start_year,m_start_month, m_start_day);
	//print_array(evt_arry_day);	
	display_event_array(evt_arry_day);
	g_array_free(evt_arry_day, FALSE); //clear the array 
	
	//update calendar	
	custom_calendar_update(CUSTOM_CALENDAR(calendar));
	
	
}

//----------------------------------------------------------------------
static void callbk_confirm_delete_all(GtkButton *button, gpointer  user_data)
{
	//g_print("Delete All Confirmed\n");
	
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

static void callbk_delete_all(GSimpleAction *action, GVariant *parameter,  gpointer user_data)
{
	//g_print("delete all callbk\n");
	
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



//----------------------------------------------------------------------
// Speaking
//-----------------------------------------------------------------------
static void callbk_speak(GSimpleAction* action, GVariant *parameter,gpointer user_data)
{
	//g_print("Speak Event callbk\n");
	speak_events();	
}

//---------------------------------------------------------------
// playraw (threading)
//---------------------------------------------------------------


static gpointer thread_playraw(gpointer user_data)
{  
    gchar *raw_file =user_data;       
    gchar *m_sample_rate_str = g_strdup_printf("%i", m_talk_rate); 
    gchar *sample_rate_str ="-r ";    
    sample_rate_str= g_strconcat(sample_rate_str,m_sample_rate_str, NULL);     
    gchar * command_str ="aplay -c 1 -f S16_LE";
    //gchar * command_str ="aplay -c 1 -f U8";
    command_str =g_strconcat(command_str," ",sample_rate_str, " ", raw_file, NULL); 
    system(command_str); 
           
    g_mutex_unlock (&lock); //thread mutex unlock 
    return NULL; 
}


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
	//g_print("Days in month = %d\n", month_days);
	
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
		//g_print("day_diff = %d\n",day_diff);
		
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
	    //g_print("next_month = %d\n", next_month);
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


//--------------------------------------------------------------------
// calendar functions
//--------------------------------------------------------------------

static void set_holidays_on_calendar(CustomCalendar *calendar){
	
	custom_calendar_reset_holidays(CUSTOM_CALENDAR(calendar));	
	
	guint8 month_days =g_date_get_days_in_month(m_start_month,m_start_year);	
	//g_print("Days in month = %d\n", month_days);
	
	for (int day=1; day<=month_days; day++)
	{
		if (is_public_holiday(day))
		{
			custom_calendar_mark_holiday(CUSTOM_CALENDAR(calendar),day);
		}
	}
}

//----------------------------------------------------------------------
// Concatentation
//---------------------------------------------------------------------

unsigned char *rawcat(unsigned char *arrys[], unsigned int arry_size[], int arry_count) 
{	
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

unsigned int get_merge_size(unsigned int sizes_arry[], int arry_size)
{
	unsigned int total_samples=0;
	for (int i = 0; i < arry_size; i++) 
	{  
    unsigned int count =sizes_arry[i]; 
    total_samples=total_samples+count;	
    }
	return total_samples;
}

//---------------------------------------------------------------------
// Diphone Speaking 
//---------------------------------------------------------------------


GList* convert_date_to_weekday_diphone_list(int day, int month, int year) {
	
	//char* weekday_str="";
	GList* result =NULL;
	
	GDate* day_date;
	day_date = g_date_new_dmy(day, month, year);
	GDateWeekday weekday =g_date_get_weekday(day_date);
	
	 switch (weekday) {
		case G_DATE_MONDAY:		
		result =word_to_diphones("monday");	
		break;
		case G_DATE_TUESDAY:		
		result =word_to_diphones("tuesday");	
		break;
		case G_DATE_WEDNESDAY:		
		result =word_to_diphones("wednesday");	
		break;
		case G_DATE_THURSDAY:		
		result =word_to_diphones("thursday");	
		break;
		case G_DATE_FRIDAY:		
		result =word_to_diphones("friday");	
		break;
		case G_DATE_SATURDAY:	
		result =word_to_diphones("saturday");	
		break;
		case G_DATE_SUNDAY:	
		result =word_to_diphones("sunday");	
		break;
		default:
		//Unknown day of week		
		result =word_to_diphones("day");	
		break;
	    }//switch dow
	
	return result;
}

//---------------------------------------------------------------------
// Convert day number to diphone list
//---------------------------------------------------------------------
GList* convert_day_number_to_diphone_list(int day_number) {
	
	GList* result =NULL;
	
	switch (day_number) {
		case 1:
		result =word_to_diphones("first");
		break;		
		case 2:
		result =word_to_diphones("second");
		break;
		case 3:
		result =word_to_diphones("third");
		break;
		case 4:
		result =word_to_diphones("fourth");
		break;
		case 5:
		result =word_to_diphones("fifth");
		break;
		case 6:
		result =word_to_diphones("sixth");
		break;
		case 7:
		result =word_to_diphones("seventh");
		break;
		case 8:
		result =word_to_diphones("eighth");
		break;
		case 9:
		result =word_to_diphones("ninth");
		break;
		case 10:
		result =word_to_diphones("tenth");
		break;
		case 11:
		result =word_to_diphones("eleventh");
		break;
		case 12:		
		result =word_to_diphones("twelfth");	
		break;
		case 13:		
		result =word_to_diphones("thirteenth");
		break;
		case 14:			
		result =word_to_diphones("fourteenth");	
		break;
		case 15:		
		result =word_to_diphones("fifteenth");
		break;
		case 16:		
		result =word_to_diphones("sixteenth");
		break;
		case 17:		
		result =word_to_diphones("seventieth");
		break;
		case 18:		
		result =word_to_diphones("eighteenth");
		break;
		case 19:		
		result =word_to_diphones("nineteenth");
		break;
		case 20:		
		result =word_to_diphones("twentieth");
		break;
		case 21:		
		result =g_list_concat(word_to_diphones("twenty"),word_to_diphones("first"));				
		break;
		case 22:		
		result =g_list_concat(word_to_diphones("twenty"),word_to_diphones("second"));		
		break;
		case 23:
		result =g_list_concat(word_to_diphones("twenty"),word_to_diphones("third"));	
		break;
		case 24:		
		result =g_list_concat(word_to_diphones("twenty"),word_to_diphones("fourth"));
		break;
		case 25:		
		result =g_list_concat(word_to_diphones("twenty"),word_to_diphones("fifth"));
		break;
		case 26:		
		result =g_list_concat(word_to_diphones("twenty"),word_to_diphones("sixth"));
		break;
		case 27:		
		result =g_list_concat(word_to_diphones("twenty"),word_to_diphones("seventh"));
		break;
		case 28:		
		result =g_list_concat(word_to_diphones("twenty"),word_to_diphones("eighth"));
		break;
		case 29:		
		result =g_list_concat(word_to_diphones("twenty"),word_to_diphones("ninth"));
		break;
		case 30:		
		result =word_to_diphones("thirtieth");
		break;
		case 31:
		result =g_list_concat(word_to_diphones("thirty"),word_to_diphones("first"));		
		break;
		default:
		//Unknown ordinal
		result =word_to_diphones("zero");
		break;		
	  } //day switch      
		
	return result;	
}

//---------------------------------------------------------------------
// Convert month to diphone list
//---------------------------------------------------------------------
GList* convert_month_to_diphone_list(int month) {
	
	GList* result =NULL;
	
	switch(month) {
	case 1:		
		result =word_to_diphones("january");
		break;
	case 2:			
		result =word_to_diphones("february");	
		break;
	case 3:		
		result =word_to_diphones("march");
		break; 
	case 4:
		result =word_to_diphones("april");
		break; 
	case 5:		
		result =word_to_diphones("may");
		break;
	case 6:		
		result =word_to_diphones("june");
		break; 
	case 7:		
		result =word_to_diphones("july");
		break;
	case 8:		
		result =word_to_diphones("august");
		break;
	case 9:		
		result =word_to_diphones("september");
		break;
	case 10:		
		result =word_to_diphones("october");
		break;
	case 11:		
		result =word_to_diphones("november");
		break; 
	case 12:		
		result =word_to_diphones("december");
		break; 
	default:
		result =word_to_diphones("month");
	}

	return result;
}

//---------------------------------------------------------------------
// Convert event number to diphone list
//---------------------------------------------------------------------
GList* get_event_number_diphone_list(int event_number) {
	
	GList* result =NULL;
	
	if (event_number ==0) {	
	GList *noevents_list=NULL;    
	noevents_list = word_to_diphones("no");   
	noevents_list = g_list_concat(noevents_list,word_to_diphones("events")); 
	noevents_list = g_list_concat(noevents_list,word_to_diphones("today")); 
	result =noevents_list;	
	} //if
	
	else if(event_number ==1){		
		result =g_list_concat(word_to_diphones("one"),word_to_diphones("event"));
	}
	else if(event_number ==2){		
		result =g_list_concat(word_to_diphones("two"),word_to_diphones("events"));
	}
	else if(event_number ==3){
		 result =g_list_concat(word_to_diphones("three"),word_to_diphones("events")); 
	}
	else if(event_number ==4){
		result =g_list_concat(word_to_diphones("four"),word_to_diphones("events")); 
	}
	else if(event_number ==5){ 
		result =g_list_concat(word_to_diphones("five"),word_to_diphones("events"));
	
	}		
	else {
	result =g_list_concat(word_to_diphones("many"),word_to_diphones("events"));
	}
	
	return result;
}

//---------------------------------------------------------------------
// Convert event number to diphone list
//---------------------------------------------------------------------
GList* get_upcoming_number_diphone_list(int number) {
	
	GList* result =NULL;
	
	if (number ==0) {	
	GList *noevents_list=NULL;    
	noevents_list = word_to_diphones("no");   
	noevents_list = g_list_concat(noevents_list,word_to_diphones("upcoming")); 
	noevents_list = g_list_concat(noevents_list,word_to_diphones("events")); 
	result =noevents_list;	
	} //if
	
	else if(number ==1){		
		result =g_list_concat(word_to_diphones("one"),word_to_diphones("upcoming"));
		result =g_list_concat(result,word_to_diphones("event"));
	}
	else if(number ==2){		
		result =g_list_concat(word_to_diphones("two"),word_to_diphones("upcoming"));
		result =g_list_concat(result,word_to_diphones("events"));
	}
	else if(number ==3){
		 result =g_list_concat(word_to_diphones("three"),word_to_diphones("upcoming"));
		 result =g_list_concat(result,word_to_diphones("events")); 
	}
	else if(number ==4){
		result =g_list_concat(word_to_diphones("four"),word_to_diphones("upcoming")); 
		result =g_list_concat(result,word_to_diphones("events"));
	}
	else if(number ==5){ 
		result =g_list_concat(word_to_diphones("five"),word_to_diphones("upcoming"));
		result =g_list_concat(result,word_to_diphones("events"));	
	}		
	else {
	result =g_list_concat(word_to_diphones("many"),word_to_diphones("upcoming"));
	result =g_list_concat(result,word_to_diphones("events"));
	}
	
	return result;
}

//---------------------------------------------------------------------
// convert number to diphone list
//---------------------------------------------------------------------
GList* convert_number_to_diphone_list(int number) {
	
	GList* result =NULL;
	
	switch (number) {		
		case 1:
		result =word_to_diphones("one");
		break;		
		case 2:
		result =word_to_diphones("two");
		break;
		case 3:
		result =word_to_diphones("three");
		break;
		case 4:
		result =word_to_diphones("four");
		break;
		case 5:
		result =word_to_diphones("five");
		break;
		case 6:
		result =word_to_diphones("six");
		break;
		case 7:
		result =word_to_diphones("seven");
		break;
		case 8:
		result =word_to_diphones("eight");
		break;
		case 9:
		result =word_to_diphones("nine");
		break;
		case 10:
		result =word_to_diphones("ten");
		break;
		case 11:
		result =word_to_diphones("eleven");
		break;
		case 12:
		result =word_to_diphones("twelve");
		break;
		case 13:
		result =word_to_diphones("thirteen");
		break;
		case 14:
		result =word_to_diphones("fourteen");
		break;
		case 15:
		result =word_to_diphones("fifteen");
		break;
		case 16:
		result =word_to_diphones("sixteen");
		break;
		case 17:
		result =word_to_diphones("seventeen");
		break;
		case 18:
		result =word_to_diphones("eighteen");
		break;
		case 19:
		result =word_to_diphones("nineteen");
		break;
		case 20:
		result =word_to_diphones("twenty");
		break;
		case 21:
		result =g_list_concat(word_to_diphones("twenty"),word_to_diphones("one"));
		break;
		case 22:
		result =g_list_concat(word_to_diphones("twenty"),word_to_diphones("two"));
		break;
		case 23:
		result =g_list_concat(word_to_diphones("twenty"),word_to_diphones("three"));
		break;
		case 24:
		result =g_list_concat(word_to_diphones("twenty"),word_to_diphones("four"));
		break;
		case 25:
		result =g_list_concat(word_to_diphones("twenty"),word_to_diphones("five"));
		break;
		case 26:
		result =g_list_concat(word_to_diphones("twenty"),word_to_diphones("six"));
		break;
		case 27:
		result =g_list_concat(word_to_diphones("twenty"),word_to_diphones("seven"));
		break;
		case 28:
		result =g_list_concat(word_to_diphones("twenty"),word_to_diphones("eight"));
		break;
		case 29:
		result =g_list_concat(word_to_diphones("twenty"),word_to_diphones("nine"));
		break;
		case 30:
		result =word_to_diphones("thirty");
		break;
		case 31:
		result =g_list_concat(word_to_diphones("thirty"),word_to_diphones("one"));
		break;
		case 32:
		result =g_list_concat(word_to_diphones("thirty"),word_to_diphones("two"));
		break;
		case 33:
		result =g_list_concat(word_to_diphones("thirty"),word_to_diphones("three"));
		break;
		case 34:
		result =g_list_concat(word_to_diphones("thirty"),word_to_diphones("four"));
		break;
		case 35:
		result =g_list_concat(word_to_diphones("thirty"),word_to_diphones("five"));
		break;
		case 36:
		result =g_list_concat(word_to_diphones("thirty"),word_to_diphones("six"));
		break;
		case 37:
		result =g_list_concat(word_to_diphones("thirty"),word_to_diphones("seven"));
		break;
		case 38:
		result =g_list_concat(word_to_diphones("thirty"),word_to_diphones("eight"));
		break;
		case 39:
		result =g_list_concat(word_to_diphones("thirty"),word_to_diphones("nine"));
		break;
		case 40:
		result =word_to_diphones("forty");
		break;
		case 41:
		result =g_list_concat(word_to_diphones("forty"),word_to_diphones("one"));
		break;
		case 42:
		result =g_list_concat(word_to_diphones("forty"),word_to_diphones("two"));
		break;
		case 43:
		result =g_list_concat(word_to_diphones("forty"),word_to_diphones("three"));
		break;
		case 44:
		result =g_list_concat(word_to_diphones("forty"),word_to_diphones("four"));
		break;
		case 45:
		result =g_list_concat(word_to_diphones("forty"),word_to_diphones("five"));
		break;
		case 46:
		result =g_list_concat(word_to_diphones("forty"),word_to_diphones("six"));
		break;
		case 47:
		result =g_list_concat(word_to_diphones("forty"),word_to_diphones("seven"));
		break;
		case 48:
		result =g_list_concat(word_to_diphones("forty"),word_to_diphones("eight"));
		break;
		case 49:
		result =g_list_concat(word_to_diphones("forty"),word_to_diphones("nine"));
		break;
		case 50:
		result =word_to_diphones("fifty");
		break;
		case 51:
		result =g_list_concat(word_to_diphones("fifty"),word_to_diphones("one"));
		break;
		case 52:
		result =g_list_concat(word_to_diphones("fifty"),word_to_diphones("two"));
		break;
		case 53:
		result =g_list_concat(word_to_diphones("fifty"),word_to_diphones("three"));
		break;
		case 54:
		result =g_list_concat(word_to_diphones("fifty"),word_to_diphones("four"));
		break;
		case 55:
		result =g_list_concat(word_to_diphones("fifty"),word_to_diphones("five"));
		break;
		case 56:
		result =g_list_concat(word_to_diphones("fifty"),word_to_diphones("six"));
		break;
		case 57:
		result =g_list_concat(word_to_diphones("fifty"),word_to_diphones("seven"));
		break;
		case 58:
		result =g_list_concat(word_to_diphones("fifty"),word_to_diphones("eight"));
		break;
		case 59:
		result =g_list_concat(word_to_diphones("fifty"),word_to_diphones("nine"));
		break;
		
		default:		
		result =word_to_diphones("zero");
		break;		
				
	}
		
	return result;
}

//---------------------------------------------------------------------
// speak events
//---------------------------------------------------------------------
static void speak_events() {

	if(m_talk==0) return;

	int day_events_number=0;
	GList *diphone_list=NULL;
	
	GList *weekday_list=convert_date_to_weekday_diphone_list(m_start_day, m_start_month, m_start_year);
	GList *day_number_list =convert_day_number_to_diphone_list(m_start_day);
	GList *month_list=convert_month_to_diphone_list(m_start_month);
	
	diphone_list =g_list_concat(diphone_list,weekday_list);
	diphone_list =g_list_concat(diphone_list,day_number_list);
	diphone_list =g_list_concat(diphone_list,month_list);
	diphone_list =g_list_concat(diphone_list, word_to_diphones("pau"));
   
   //---------------------------------------------------------------------------------------
   // check for public holiday
   //-----------------------------------------------------------------------------------------

	if ((m_holidays ==1) && (is_public_holiday(m_start_day)))
	{
		//GList *public_holiday_list=NULL;
		
		gchar* hol_str =get_holiday_str(m_start_day);
		//g_print("holiday = %s",hol_str);
		
		GList *hol_word_list=NULL;  
		
		gchar** hwords;		 
		hwords = g_strsplit (hol_str, " ", 0); //split on space
		int j=0;			   
		while(hwords[j] != NULL)
		{	
		hol_word_list=word_to_diphones(hwords[j]);
		diphone_list =g_list_concat(diphone_list,hol_word_list);	
		j++;
		} //while loop words
		
		diphone_list =g_list_concat(diphone_list, word_to_diphones("pau"));
		
		//public_holiday_list =g_list_concat(public_holiday_list,word_to_diphones("public"));
		//public_holiday_list =g_list_concat(public_holiday_list, word_to_diphones("holiday"));
		//diphone_list =g_list_concat(diphone_list,public_holiday_list);	

	}

   
	GArray *day_events_arry =g_array_new(FALSE, FALSE, sizeof(CALENDAR_TYPE_EVENT));
	db_get_all_events_year_month_day(day_events_arry , m_start_year,m_start_month, m_start_day);
	
	//-------------------------------------------------------------------
	// Get event number
	//--------------------------------------------------------------------	
	
	int event_count = day_events_arry->len;

	if(m_talk_event_number) {
		GList* event_number_list =get_event_number_diphone_list(event_count);
	    diphone_list =g_list_concat(diphone_list,event_number_list);
	    diphone_list =g_list_concat(diphone_list, word_to_diphones("pau"));		
	}


	//-----------------------------------------------------------------------
	// now loop through day events
	//------------------------------------------------------------------------

	
	for (int i = 0; i < day_events_arry->len; i++)
	{

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
		gint has_reminder = 0;
		gint reminder_min = 0;

		CalendarEvent *evt = g_array_index(day_events_arry, CalendarEvent *, i);

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

		gchar *start_time_str = "";
		gchar *time_str = "";
		gchar *starthour_str = "";
		gchar *startmin_str = "";
		gchar *endhour_str = "";
		gchar *endmin_str = "";
		gchar *ampm_str = "";

	//--------------------------------------------------------------
	// Talk Time
	//---------------------------------------------------------------

	if (m_talk_time) {
	
	 GList* time_list=NULL;
	 
		if(is_allday) {			
		
		time_list =g_list_concat(time_list,word_to_diphones("all"));
		time_list =g_list_concat(time_list, word_to_diphones("day"));
		time_list =g_list_concat(time_list, word_to_diphones("event")); 
		
		}
		
		else {

		GList* hour_list=NULL;
	  	GList* min_list=NULL;

		//g_print("time format %i\n", m_12hour_format);
		//g_print("start_hour = %i\n",start_hour);
		//g_print("start_min = %i\n",start_min);
		//hour_list =convert_number_to_diphone_list(start_hour);

			if (m_12hour_format)
			{
				if (start_hour >= 13 && start_hour <= 23)
				{
					int s_hour = start_hour - 12;

					//g_print("s_hour = %i\n",s_hour);
					ampm_str = "pmm";
					//starthour_str = convert_number_to_cardinal_string(s_hour);
					//g_list_free(hour_list);
					hour_list =convert_number_to_diphone_list(s_hour);
				}
				if(start_hour ==12)
				{
					ampm_str = "pmm";
					//starthour_str = convert_number_to_cardinal_string(start_hour);
					//g_list_free(hour_list);
					hour_list =convert_number_to_diphone_list(start_hour);
				}
			   if(start_hour <12)
				{
					ampm_str = "amm";
					//starthour_str = convert_number_to_cardinal_string(start_hour);
					//g_list_free(hour_list);
					hour_list =convert_number_to_diphone_list(start_hour);
				}
			} // 12
			if(!m_12hour_format) //24 hours
			{
				//starthour_str = convert_number_to_cardinal_string(start_hour);
				hour_list =convert_number_to_diphone_list(start_hour);
			} // 24
		
			if (start_min > 0 && start_min < 10)
			{
				//time_str = g_strconcat(time_str, "zero ", startmin_str, "   ", NULL);
			 	GList* zero_list=convert_number_to_diphone_list(0);
				GList* start_min_list =convert_number_to_diphone_list(start_min);
		        min_list =g_list_concat(zero_list,start_min_list);
				//time_list =g_list_concat(hour_list,zero_list);

			}
			else if(start_min >=10)
			{
				min_list =convert_number_to_diphone_list(start_min);
			}

	
	 
	 time_list =g_list_concat(hour_list,min_list);
	} //else not all day

	//add am or pm diphone list
	time_list =g_list_concat(time_list, word_to_diphones("pau"));
	time_list =g_list_concat(time_list,word_to_diphones(ampm_str));	
	time_list =g_list_concat(time_list, word_to_diphones("pau"));	
	diphone_list =g_list_concat(diphone_list,time_list);
	
	
	} //end talk time------------------------------------------


	//----------------------------------------------------------
	// talk summary title
	//-----------------------------------------------------------
	GList *event_title_list=NULL;  
    char* str = remove_semicolons(summary_str);
  
	gchar** words;		 
	words = g_strsplit (str, " ", 0); //split on space
	int j=0;			   
	while(words[j] != NULL)
	{	
	event_title_list=word_to_diphones(words[j]);
	diphone_list =g_list_concat(diphone_list,event_title_list);	
	j++;
	} //while loop words
   
    if(g_list_length(event_title_list) ==0){		
		event_title_list=g_list_concat(event_title_list,word_to_diphones("unknown"));
		//event_title_list=g_list_concat(event_title_list,word_to_diphones("entry"));		
		diphone_list =g_list_concat(diphone_list,event_title_list);
	}
    diphone_list =g_list_concat(diphone_list, word_to_diphones("pau"));


	//----------------------------------------------------------
	// talk location
	//-----------------------------------------------------------

	if(m_talk_location) {
	GList *loc_list=NULL;  
    char* loc_str = remove_semicolons(location_str);
  
	gchar** loc_words;		 
	loc_words = g_strsplit (loc_str, " ", 0); //split on space
	int jj=0;			   
	while(loc_words[jj] != NULL)
	{	
	loc_list=word_to_diphones(loc_words[jj]);
	diphone_list =g_list_concat(diphone_list,loc_list);	
	jj++;
	} //while loop words
   
    if(g_list_length(loc_list) ==0){		
		loc_list=g_list_concat(loc_list,word_to_diphones("unknown"));
		loc_list=g_list_concat(loc_list,word_to_diphones("location"));		
		diphone_list =g_list_concat(diphone_list,loc_list);
	}
    diphone_list =g_list_concat(diphone_list, word_to_diphones("pau"));
	}//m_talk_location

	//---------------------------------------------------------------------
	// Talk Priority
	//----------------------------------------------------------------------

	if (m_talk_priority)
	{
		if (is_priority == 1)
		{
		GList *high_priority_list=NULL;
		high_priority_list =g_list_concat(high_priority_list,word_to_diphones("high"));
		high_priority_list =g_list_concat(high_priority_list, word_to_diphones("priority"));
		diphone_list =g_list_concat(diphone_list,high_priority_list);
		}
	}

	} // day_events for loop
	
	
//------------------------------------------------------------------
// Talk upcoming events (experimental)
//------------------------------------------------------------------
	
	GDate *today_date;
	today_date = g_date_new();
	g_date_set_time_t(today_date, time(NULL));
	int today = g_date_get_day(today_date);
	int month = g_date_get_month(today_date);
	int year = g_date_get_year(today_date);
	g_date_free(today_date); // freeit quick
	
	if(m_talk_upcoming && m_start_day==today && m_start_month ==month && m_start_year==year)  
	{
	
	GArray *evts_upcoming = get_upcoming_array(7);
	
	int num_upcoming = evts_upcoming->len;
	//g_print("upcoming number = %d\n",num_upcoming);
	
	GList* upcoming_number_list =get_upcoming_number_diphone_list(num_upcoming);
	diphone_list =g_list_concat(diphone_list,upcoming_number_list);
	diphone_list =g_list_concat(diphone_list, word_to_diphones("pau"));
	
	for (int i = 0; i < evts_upcoming->len; i++)
	{
	gint evt_id = 0;
	gchar *summary_str = "";
	gint start_day=1;
	gint start_month=1;
	gint start_year=2023;
		
	CalendarEvent *evt = g_array_index(evts_upcoming, CalendarEvent *, i);
	
	g_object_get(evt, "eventid", &evt_id, NULL);
	g_object_get(evt, "summary", &summary_str, NULL);
	//g_object_get(evt, "location", &location_str, NULL);
	//g_object_get(evt, "description", &description_str, NULL);
	g_object_get(evt, "startyear", &start_year, NULL);
	g_object_get(evt, "startmonth", &start_month, NULL);
	g_object_get(evt, "startday", &start_day, NULL);
	
	
	GList *weekday_list=convert_date_to_weekday_diphone_list(start_day, start_month, start_year);
	GList *day_number_list =convert_day_number_to_diphone_list(start_day);
	GList *month_list=convert_month_to_diphone_list(start_month);
	
	diphone_list =g_list_concat(diphone_list,weekday_list);
	diphone_list =g_list_concat(diphone_list,day_number_list);
	diphone_list =g_list_concat(diphone_list,month_list);
	diphone_list =g_list_concat(diphone_list, word_to_diphones("pau"));
	
	GList *event_title_list=NULL;  
    char* str = remove_semicolons(summary_str);
	
	gchar** words;		 
	words = g_strsplit (str, " ", 0); //split on space
	int j=0;			   
	while(words[j] != NULL)
	{	
	event_title_list=word_to_diphones(words[j]);
	diphone_list =g_list_concat(diphone_list,event_title_list);	
	j++;
	} //while loop words
	
    diphone_list =g_list_concat(diphone_list, word_to_diphones("pau"));
	
	} //for evts upcoming
	
	}//if upcoming
	
		
	gpointer diphone_list_pointer;
	gchar* diphone_str;		
	gint diphone_number  =g_list_length(diphone_list);
	//g_print("diphone_number = %i\n", diphone_number);
	//create diphone array using list length
	unsigned char *diphone_arrays[diphone_number]; 
	unsigned int diphone_arrays_sizes[diphone_number];
		
	//load diphone arrays
	for(int i=0; i <diphone_number; i++)
	{
		diphone_list_pointer=g_list_nth_data(diphone_list,i);
		diphone_str=(gchar *)diphone_list_pointer;					
		//g_print("diphone = %s\n",diphone_str);		
		diphone_arrays[i] = load_diphone_arry(diphone_str);
		//g_print("diphone length = %i\n",load_diphone_len(diphone_str));
		diphone_arrays_sizes[i]=load_diphone_len(diphone_str);		
	}	
	
	//concatenate using raw cat
	unsigned char *data = rawcat(diphone_arrays, diphone_arrays_sizes, diphone_number);	
	unsigned int data_len = get_merge_size(diphone_arrays_sizes,diphone_number);	
    
		
	gchar* raw_file ="/tmp/talkout.raw";
	FILE* f = fopen(raw_file, "w");
    fwrite(data, data_len, 1, f);
    fclose(f); 
    
	GThread *thread_audio; 
	g_mutex_lock (&lock);
    thread_audio = g_thread_new(NULL, thread_playraw, raw_file);  
	g_thread_unref (thread_audio);
	
	//clean up 
	g_list_free(diphone_list);	
	free(data);	//prevent memory leak
}



//----------------------------------------------------------------------
//Implementation of Calendar menu callbks
//----------------------------------------------------------------------

//----------------------------------------------------------------
// Callback home (go to current date)
//-----------------------------------------------------------------
static void callbk_calendar_home(GSimpleAction * action, GVariant *parameter, gpointer user_data)
{
	GtkWindow *window =user_data;	
	GtkWidget *calendar =g_object_get_data(G_OBJECT(window), "window-calendar-key");
	GtkWidget *label_date =g_object_get_data(G_OBJECT(window), "window-label-date-key");
	
	custom_calendar_goto_today(CUSTOM_CALENDAR(calendar));
	
	m_today_day = custom_calendar_get_day(CUSTOM_CALENDAR(calendar));
	m_today_month = custom_calendar_get_month(CUSTOM_CALENDAR(calendar));
	m_today_year = custom_calendar_get_year(CUSTOM_CALENDAR(calendar));		
	//g_print("Today is : %d-%d-%d \n", m_today_day, m_today_month,m_today_year);

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
	//print_array(evt_arry_month);
	set_marks_on_calendar(CUSTOM_CALENDAR(calendar),evt_arry_month);
	g_array_free(evt_arry_month, FALSE); //clear the array
	
	GArray *evt_arry_day;
	//g_print("year = %d, month = %d day = %d\n",year,month,day);
	evt_arry_day = g_array_new(FALSE, FALSE, sizeof(CALENDAR_TYPE_EVENT)); // setup arraylist
	db_get_all_events_year_month_day(evt_arry_day, m_start_year,m_start_month, m_start_day);
	//print_array(evt_arry_day);	
	display_event_array(evt_arry_day);
	g_array_free(evt_arry_day, FALSE); //clear the array 
	
	custom_calendar_update(CUSTOM_CALENDAR(calendar));		
}

//---------------------------------------------------------------------

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
	m_todaycolour=g_ascii_strdown(todaycolour,-1);
	//remove unwanted chars?
     	
	buffer_eventcolour = gtk_entry_get_buffer(GTK_ENTRY(entry_eventcolour));
	const gchar* eventcolour = gtk_entry_buffer_get_text(buffer_eventcolour);
	m_eventcolour=g_ascii_strdown(eventcolour,-1);
	
	buffer_holidaycolour = gtk_entry_get_buffer(GTK_ENTRY(entry_holidaycolour));
	const gchar* holidaycolour = gtk_entry_buffer_get_text(buffer_holidaycolour);
	m_holidaycolour=g_ascii_strdown(holidaycolour,-1);
	
	//g_print("today colour = %s\n",m_todaycolour);
	//g_print("event colour = %s\n",m_eventcolour);
	//g_print("holiday colour = %s\n",m_holidaycolour);
	
	g_object_set(calendar, "todaycolour", m_todaycolour, NULL);
	g_object_set(calendar, "eventcolour", m_eventcolour, NULL);
	g_object_set(calendar, "holidaycolour", m_holidaycolour, NULL);	
	custom_calendar_update(CUSTOM_CALENDAR(calendar));
	
	//save preferences
	config_write();		
	gtk_window_destroy(GTK_WINDOW(dialog));
}

static void callbk_calendar_colour(GSimpleAction * action, GVariant *parameter, gpointer user_data)
{
	//g_print("change colour callbk");
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

//----------------------------------------------------------------------
static void callbk_calendar_speaktime(GSimpleAction * action, GVariant *parameter, gpointer user_data)
{
	//g_print("speak time callbk");
	GtkWidget *window = user_data;
	
	GDateTime *dt = g_date_time_new_now_local(); 
	gint hour =g_date_time_get_hour(dt);	
	gint min= g_date_time_get_minute(dt);
	
	speak_time(hour,min);
	//g_print("time = %d:%d\n",hour,min);	
    g_date_time_unref (dt);
}

//---------------------------------------------------------------------
// Update Date Label
//----------------------------------------------------------------------

static void update_label_date(CustomCalendar *calendar, gpointer user_data)
{
GtkWidget *label_date = (GtkWidget *) user_data;
	//g_print("Day is : %d-%d-%d \n", m_start_day, m_start_month,m_start_year);
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

	 // //gchar *day_str = g_strdup_printf ("%02d",m_start_day);
	 // //gchar *year_str = g_strdup_printf ("%02d", m_start_year);

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
		
	if ((m_holidays ==1) && (is_public_holiday(m_start_day)))
	{
		//date_str =g_strconcat(date_str,"#", NULL); //public holiday
		gchar * holiday_str = get_holiday_str(m_start_day);	
		date_str =g_strconcat(date_str," ",holiday_str, NULL); 	
	 	//date_str =g_strconcat(date_str," ",holiday_str,"#", NULL);
	}

	
	 int event_num =get_number_of_day_events();
	 if(event_num>0) {
	 	date_str =g_strconcat(date_str,"*", NULL);
	 }
	 
	gtk_label_set_text(GTK_LABEL(label_date), date_str);

	
	 
}
//----------------------------------------------------------------------
// Calendar callbks
//----------------------------------------------------------------------

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
	//print_array(evt_arry_month);
	set_marks_on_calendar(CUSTOM_CALENDAR(calendar),evt_arry_month);
	g_array_free(evt_arry_month, FALSE); //clear the array
	
	custom_calendar_update(CUSTOM_CALENDAR(calendar));	
}

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
	//print_array(evt_arry_month);
	set_marks_on_calendar(CUSTOM_CALENDAR(calendar),evt_arry_month);
	g_array_free(evt_arry_month, FALSE); //clear the array
		
	custom_calendar_update(CUSTOM_CALENDAR(calendar));
}

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
	//print_array(evt_arry_month);
	set_marks_on_calendar(CUSTOM_CALENDAR(calendar),evt_arry_month);
	g_array_free(evt_arry_month, FALSE); //clear the array
	custom_calendar_update(CUSTOM_CALENDAR(calendar));
	
}

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
	//print_array(evt_arry_month);
	set_marks_on_calendar(CUSTOM_CALENDAR(calendar),evt_arry_month);
	g_array_free(evt_arry_month, FALSE); //clear the array
	
	custom_calendar_update(CUSTOM_CALENDAR(calendar));	
}


static void callbk_calendar_day_selected(CustomCalendar *calendar, gpointer user_data)
{
	GtkWidget *label_date = (GtkWidget *)user_data;
	
	m_start_day = custom_calendar_get_day(CUSTOM_CALENDAR(calendar));
	m_start_month = custom_calendar_get_month(CUSTOM_CALENDAR(calendar));
	m_start_year = custom_calendar_get_year(CUSTOM_CALENDAR(calendar));
	//g_print("Date is : %d-%d-%d \n", m_start_day, m_start_month,m_start_year);
			
	g_list_store_remove_all(m_store); // clear listbox store
	
	//mark up calendar	
	if(m_holidays) set_holidays_on_calendar(CUSTOM_CALENDAR(calendar));
	
	update_label_date(CUSTOM_CALENDAR(calendar), label_date);		
	
	GArray *evt_arry_day; //normal day events
	//g_print("year = %d, month = %d day = %d\n",year,month,day);
	evt_arry_day = g_array_new(FALSE, FALSE, sizeof(CALENDAR_TYPE_EVENT)); // setup arraylist
	db_get_all_events_year_month_day(evt_arry_day, m_start_year,m_start_month, m_start_day);
	//print_array(evt_arry_day);
	
	display_event_array(evt_arry_day); //display listbox day events
	g_array_free(evt_arry_day, FALSE); //clear the array 
	
	custom_calendar_update(CUSTOM_CALENDAR(calendar));				
}



//--------------------------------------------------------------------
// calendar functions
//--------------------------------------------------------------------

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


//----------------------------------------------------------------------

//----------------------------------------------------------------------
//Implementation of Help menu callbks
//----------------------------------------------------------------------

static void callbk_info(GSimpleAction *action, GVariant *parameter,  gpointer user_data){
	//g_print("Info callbk\n");
	
	GtkWidget *window =user_data;
	GtkWidget *dialog;
	GtkWidget *box;
	gint response;
	
	GtkWidget *label_record_info;
	GtkWidget *label_record_number;
	GtkWidget *label_sqlite_version;
	
	GtkWidget *label_font_info;	
	GtkWidget *label_desktop_font;
	GtkWidget *label_gnome_text_scale;
	
		
	GtkWidget *label_keyboard_shortcuts;
	GtkWidget *label_speak_shortcut;
	GtkWidget *label_home_shortcut;
	GtkWidget *label_newevent_shortcut;
	GtkWidget *label_editevent_shortcut;
	
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
	
	label_speak_shortcut=gtk_label_new("SPACEBAR: Speak");
	label_home_shortcut=gtk_label_new("HOME: Go to today");
	label_newevent_shortcut=gtk_label_new("CTRL+e: New event");
	label_editevent_shortcut=gtk_label_new("CTRL+n: Edit selected event");
	
	
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
	gtk_box_append(GTK_BOX(box), label_speak_shortcut);
	
	gtk_box_append(GTK_BOX(box), label_record_info);
	gtk_box_append(GTK_BOX(box), label_record_number);
	gtk_box_append(GTK_BOX(box), label_sqlite_version);
	
	gtk_box_append(GTK_BOX(box),label_font_info);
	gtk_box_append(GTK_BOX(box),label_desktop_font);
	gtk_box_append(GTK_BOX(box),label_gnome_text_scale);
	
	//gtk_box_append(GTK_BOX(box),label_work_dir);
	//gtk_box_append(GTK_BOX(box),label_working_dir);	
	//pango_attr_list_unref (attrs);
	
	pango_attr_list_unref(attrs);
	gtk_window_present (GTK_WINDOW (dialog));
	
}
static void callbk_about(GSimpleAction* action, GVariant *parameter, gpointer user_data){
	//g_print("About callbk\n");
	GtkWidget *window = user_data;
	const gchar *authors[] = {"Alan Crispin", NULL};
	GtkWidget *about_dialog;
	about_dialog = gtk_about_dialog_new();
	gtk_window_set_transient_for(GTK_WINDOW(about_dialog),GTK_WINDOW(window));
	gtk_widget_set_size_request(about_dialog, 200,200);
    gtk_window_set_modal(GTK_WINDOW(about_dialog),TRUE);
	gtk_about_dialog_set_program_name(GTK_ABOUT_DIALOG(about_dialog), "Talk Calendar");
	gtk_about_dialog_set_version (GTK_ABOUT_DIALOG(about_dialog), "Version 0.2.4");
	gtk_about_dialog_set_copyright(GTK_ABOUT_DIALOG(about_dialog),"Copyright © 2024");
	gtk_about_dialog_set_comments(GTK_ABOUT_DIALOG(about_dialog),"Talking calendar. Diphone voice. Sqlite backend.");
	gtk_about_dialog_set_license_type (GTK_ABOUT_DIALOG(about_dialog), GTK_LICENSE_LGPL_2_1);
	gtk_about_dialog_set_website(GTK_ABOUT_DIALOG(about_dialog),"https://github.com/crispinprojects/");
	gtk_about_dialog_set_website_label(GTK_ABOUT_DIALOG(about_dialog),"Talk Calendar Website");
	gtk_about_dialog_set_authors(GTK_ABOUT_DIALOG(about_dialog), authors);
	gtk_about_dialog_set_logo_icon_name(GTK_ABOUT_DIALOG(about_dialog), "x-office-calendar");
	gtk_widget_set_visible (about_dialog, TRUE);	
}

//----------------------------------------------------------------------

//----------------------------------------------------------------------
// LISTBOX functions and callbks
//----------------------------------------------------------------------

static GtkWidget *create_widget (gpointer item, gpointer user_data)
{
  DisplayItem *obj = (DisplayItem *)item;
  GtkWidget *label;
  label = gtk_label_new (""); 
  gtk_widget_set_halign (GTK_WIDGET (label), GTK_ALIGN_START); //left align 
  g_object_bind_property (obj, "label", label, "label", G_BINDING_SYNC_CREATE);
  return label;
}

static void add_separator (GtkListBoxRow *row, GtkListBoxRow *before, gpointer data)
{	
  if (!before) return;
  gtk_list_box_row_set_header (row, gtk_separator_new (GTK_ORIENTATION_HORIZONTAL));
}

static void callbk_row_activated (GtkListBox *listbox,GtkListBoxRow *row, gpointer user_data)
{
	m_row_index = gtk_list_box_row_get_index (row);
	//g_print("m_row_index = %d\n",m_row_index);
	DisplayItem *obj = g_list_model_get_item (G_LIST_MODEL (m_store), m_row_index);
	if(obj==NULL) return;
	gint id_value;
	gchar* label_value;
	g_object_get (obj, "id", &id_value, NULL);
	g_object_get (obj, "label", &label_value, NULL);
	m_id_selection=id_value;
	//g_print("m_id_selection = %d\n",m_id_selection);
	gint evt_id=0;
	gchar *summary_str="";
	gchar *location_str="";		
	//CalendarEvent *tmp_evt = g_object_new(CALENDAR_TYPE_EVENT, 0);	
	selected_evt = g_object_new(CALENDAR_TYPE_EVENT, 0);
	db_get_event(m_id_selection, selected_evt);
	
	g_object_get (selected_evt, "eventid", &evt_id, NULL);
	g_object_get (selected_evt, "summary", &summary_str, NULL);
	g_object_get (selected_evt, "location", &location_str, NULL);	
	//g_print("selected event: id = %d summary = %s location = %s\n",evt_id, summary_str, location_str);		
}

//-----------------------------------------------------------------
// Display events (time order)
//-----------------------------------------------------------------

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
      //create display str      
      gchar *display_str = "";
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
				if(start_hour  == 12)
				{
					ampm_str = "pm ";					
					starthour_str = g_strdup_printf("%d", start_hour);
				}
				
				 if(start_hour <12)
				{
					ampm_str = "am ";
					starthour_str = g_strdup_printf("%d", start_hour);
				}			
				
			} // 12hout format
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
			if (m_show_location)
			{
				if (strlen(location_str) == 0)
				{
					//db sorted
					display_str = g_strconcat(display_str, time_str, summary_str, ". ", NULL);
				}
				else
				{
					display_str = g_strconcat(display_str, time_str, summary_str, ". ",location_str, ".", NULL);
				}
			}
			else
			{
				display_str = g_strconcat(display_str, time_str, summary_str, ".", NULL);
			}

			if (is_priority)
			{
				display_str = g_strconcat(display_str, " High Priority.", NULL);
			}

			if (!strlen(description_str) == 0){
			display_str = g_strconcat(display_str, "\n", description_str, NULL);
			}

			// Display day events

			if (is_allday)
			{
				start_hour = 0;
				start_min = 0;
			}
			//start_time_sort = start_hour * 60 * 60 + 60 * start_min; // seconds

			DisplayItem *item = NULL;
			item = g_object_new(display_item_get_type(),
								"id", evt_id,
								"label", display_str,								
								NULL);			
			
			g_list_store_append (m_store, item); //db sorted
			g_object_unref(item);		
		}//evt_arry loop
}

//----------------------------------------------------------------------

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

//----------------------------------------------------------------------
// Speak time
//----------------------------------------------------------------------

static void speak_time(gint hour, gint min) 
{
	//g_print("speak time = %d:%d\n",hour,min);
			
	GList *diphone_list=NULL;
	GList* time_list=NULL;
	GList* hour_list=NULL;
	GList* min_list=NULL;	
	
	gchar *time_str = "";
	gchar *hour_str = "";
	gchar *min_str = "";		
	gchar *ampm_str = "";
	
	diphone_list =g_list_concat(word_to_diphones("the"),word_to_diphones("time"));
	diphone_list =g_list_concat(diphone_list,word_to_diphones("is"));	
    diphone_list =g_list_concat(diphone_list,word_to_diphones("pau"));
	
	if (m_12hour_format)
			{
				if (hour >= 13 && hour <= 23)
				{
					int s_hour = hour - 12;
					ampm_str = "pmm";					
					hour_list =convert_number_to_diphone_list(s_hour);
				}
			   if(hour == 12)
				{
					ampm_str = "pmm";					
					hour_list =convert_number_to_diphone_list(hour);
				}
			   if(hour <12)
				{
					ampm_str = "amm";					
					hour_list =convert_number_to_diphone_list(hour);
				}
			} // 12
			if(!m_12hour_format) //24 hours
			{
				hour_list =convert_number_to_diphone_list(hour);
			} // 24

			if (min > 0 && min < 10)
			{				
			 	GList* zero_list=convert_number_to_diphone_list(0);
				GList* zero_min_list =convert_number_to_diphone_list(min);
		        min_list =g_list_concat(zero_list,zero_min_list);
			}
			else if(min >=10)
			{
				min_list =convert_number_to_diphone_list(min);
			}

	if (m_12hour_format) {	
		
		time_list =g_list_concat(hour_list, word_to_diphones("pau"));
		time_list =g_list_concat(time_list,min_list);
		time_list =g_list_concat(time_list, word_to_diphones("pau"));
	    time_list =g_list_concat(time_list,word_to_diphones(ampm_str));		   
	    time_list =g_list_concat(time_list, word_to_diphones("pau"));	
	}
	else {
		time_list =g_list_concat(hour_list, word_to_diphones("pau"));
		time_list =g_list_concat(time_list,min_list);
	}
	  	
	diphone_list =g_list_concat(diphone_list,time_list);
		
	
	gpointer diphone_list_pointer;
	gchar* diphone_str;		
	gint diphone_number  =g_list_length(diphone_list);
	//g_print("diphone_number = %i\n", diphone_number);
	//create diphone array using list length
	unsigned char *diphone_arrays[diphone_number]; 
	unsigned int diphone_arrays_sizes[diphone_number];
		
	//load diphone arrays
	for(int i=0; i <diphone_number; i++)
	{
		diphone_list_pointer=g_list_nth_data(diphone_list,i);
		diphone_str=(gchar *)diphone_list_pointer;					
		//g_print("diphone = %s\n",diphone_str);		
		diphone_arrays[i] = load_diphone_arry(diphone_str);
		//g_print("diphone length = %i\n",load_diphone_len(diphone_str));
		diphone_arrays_sizes[i]=load_diphone_len(diphone_str);		
	}	
	
	//concatenate using raw cat
	unsigned char *data = rawcat(diphone_arrays, diphone_arrays_sizes, diphone_number);	
	unsigned int data_len = get_merge_size(diphone_arrays_sizes,diphone_number);	
    
		
	gchar* raw_file ="/tmp/talkout.raw";
	FILE* f = fopen(raw_file, "w");
    fwrite(data, data_len, 1, f);
    fclose(f); 
    
	GThread *thread_audio; 
	g_mutex_lock (&lock);
    thread_audio = g_thread_new(NULL, thread_playraw, raw_file);  
	g_thread_unref (thread_audio);
	
	//clean up 
	g_list_free(diphone_list);	
	free(data);	//prevent memory leak as malloc used
	
}



//-----------------------------------------------------------------------
static GMenu *create_menu(const GtkApplication *app) {
	
		
	GMenu *menu;
    GMenu *file_menu;
    GMenu *edit_menu;
    GMenu *event_menu;
    GMenu *calendar_menu;
    GMenu *help_menu;
    GMenuItem *item;

	menu =g_menu_new();
	file_menu =g_menu_new();
	edit_menu =g_menu_new();
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
	
	//Edit items
	item =g_menu_item_new("Preferences", "app.preferences");
	g_menu_append_item(edit_menu,item);
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
	
	item =g_menu_item_new("Colour", "app.colour");
	g_menu_append_item(calendar_menu,item);
	g_object_unref(item);
	
	item =g_menu_item_new("Speak Time", "app.speaktime");
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
    g_menu_append_submenu(menu, "Edit", G_MENU_MODEL(edit_menu));
    g_object_unref(edit_menu);
    g_menu_append_submenu(menu, "Event", G_MENU_MODEL(event_menu));
    g_object_unref(event_menu);
    g_menu_append_submenu(menu, "Calendar", G_MENU_MODEL(calendar_menu));
    g_object_unref(calendar_menu);
    g_menu_append_submenu(menu, "Help", G_MENU_MODEL(help_menu));
    g_object_unref(help_menu);
    
    return menu;
}

void callbk_shutdown(GtkWindow *window, gint response_id, gpointer user_data)
{
	//g_print("shutdown called\n");
	gtk_window_get_default_size(GTK_WINDOW(window), &m_window_width,&m_window_height);	
	config_write(); 
}

static void startup(GtkApplication *app)
{
	 //g_print("startup  called\n");
	 config_initialize();	
	 db_create_events_table(); //startup database 
	 
}



static void activate (GtkApplication* app, gpointer user_data)
{
	GtkWidget *window;	
	GtkWidget *box;
	GtkWidget *calendar; 
	GtkWidget *sw; //scrolled window
	GtkWidget* listbox;
	GtkWidget *label_date; //display date	
	GMenu *menu;
	
	//Keyboard accelerators
	const gchar *home_accels[2] = { "Home", NULL };
	const gchar *speak_accels[2] = { "space", NULL };
	const gchar *newevent_accels[2] = {"<Ctrl>n", NULL };	
	const gchar *editevent_accels[2] = {"<Ctrl>e", NULL };
	const gchar *time_accels[2] = {"t", NULL };
	const gchar *info_accels[2] = {"F1", NULL };
	const gchar *delete_accels[2] = {"Delete", NULL };	
	const gchar * preferences_accels[2] = { "<Ctrl><Alt>P", NULL };
	const gchar * quit_accels[2] = { "<Ctrl>Q", NULL };
	
	
	//setup window
	window = gtk_application_window_new (app);
	//guint id =gtk_application_window_get_id(GTK_APPLICATION_WINDOW(window));
	//g_print("Window ID = %u\n",id);	
	gtk_window_set_title (GTK_WINDOW (window), "Talk Calendar");	
	gtk_window_set_default_size (GTK_WINDOW (window),m_window_width,m_window_height);
	g_signal_connect (window, "destroy", G_CALLBACK (callbk_shutdown), NULL);
	
	//setup date label
	label_date = gtk_label_new("");
	gtk_label_set_xalign(GTK_LABEL(label_date), 0.5);
	
	//setup calendar	
	calendar = custom_calendar_new();
	m_start_day = custom_calendar_get_day(CUSTOM_CALENDAR(calendar));
	m_start_month = custom_calendar_get_month(CUSTOM_CALENDAR(calendar));
	m_start_year = custom_calendar_get_year(CUSTOM_CALENDAR(calendar));
	//g_print("Custom Calendar Date: %d-%d-%d \n", m_start_day, m_start_month, m_start_year);	
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
   	
	
	//ACTIONS
	
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
		
	//Calendar Actions	
	GSimpleAction *home_action;	
	home_action=g_simple_action_new("home",NULL); //app.home
	g_action_map_add_action(G_ACTION_MAP(app), G_ACTION(home_action)); //make visible	
	g_signal_connect(home_action, "activate",  G_CALLBACK(callbk_calendar_home), window);
	
	GSimpleAction *colour_action;	
	colour_action=g_simple_action_new("colour",NULL); //app.colour
	g_action_map_add_action(G_ACTION_MAP(app), G_ACTION(colour_action)); //make visible	
	g_signal_connect(colour_action, "activate",  G_CALLBACK(callbk_calendar_colour), window);
	
	GSimpleAction *speaktime_action;	
	speaktime_action=g_simple_action_new("speaktime",NULL); //app.speaktime
	g_action_map_add_action(G_ACTION_MAP(app), G_ACTION(speaktime_action)); //make visible	
	g_signal_connect(speaktime_action, "activate",  G_CALLBACK(callbk_calendar_speaktime), window);
			
	//Help Actions
	GSimpleAction *info_action;
	info_action=g_simple_action_new("info",NULL); //app.info
	g_action_map_add_action(G_ACTION_MAP(app), G_ACTION(info_action)); //make visible
	g_signal_connect(info_action, "activate",  G_CALLBACK(callbk_info), window);
	
	GSimpleAction *about_action;
	about_action=g_simple_action_new("about",NULL); //app.about
	g_action_map_add_action(G_ACTION_MAP(app), G_ACTION(about_action)); //make visible
	g_signal_connect(about_action, "activate",  G_CALLBACK(callbk_about), window);
	
				
	menu=create_menu(app);	
	gtk_application_set_menubar (app,G_MENU_MODEL(menu));
    gtk_application_window_set_show_menubar(GTK_APPLICATION_WINDOW(window), TRUE);

	// connect keyboard accelerators	
	gtk_application_set_accels_for_action(GTK_APPLICATION(app),"app.home", home_accels);
	gtk_application_set_accels_for_action(GTK_APPLICATION(app),"app.speak", speak_accels);
	gtk_application_set_accels_for_action(GTK_APPLICATION(app),"app.newevent", newevent_accels);
	gtk_application_set_accels_for_action(GTK_APPLICATION(app),"app.editevent", editevent_accels);
	gtk_application_set_accels_for_action(GTK_APPLICATION(app),"app.deleteevent", delete_accels);	
	gtk_application_set_accels_for_action(GTK_APPLICATION(app),"app.speaktime", time_accels);
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
	
		
	//g_object_set(calendar, "todaycolour", "red", NULL);
	//g_object_set(calendar, "eventcolour", "purple", NULL);
	//g_object_set(calendar, "holidaycolour", "green", NULL);	
	
	g_object_set(calendar, "todaycolour", m_todaycolour, NULL);
	g_object_set(calendar, "eventcolour", m_eventcolour, NULL);
	g_object_set(calendar, "holidaycolour", m_holidaycolour, NULL);	
	
	//m_frame =FALSE;	
	g_object_set(calendar, "frame", m_frame, NULL);	
	
	custom_calendar_update(CUSTOM_CALENDAR(calendar));
	custom_calendar_goto_today(CUSTOM_CALENDAR(calendar));
	custom_calendar_update(CUSTOM_CALENDAR(calendar));
	
	//Testing
	//custom_calendar_mark_day(CUSTOM_CALENDAR(calendar), 14);
	//custom_calendar_reset_marks(CUSTOM_CALENDAR(calendar));
	//custom_calendar_mark_holiday(CUSTOM_CALENDAR(calendar),25);
	//custom_calendar_reset_holidays(CUSTOM_CALENDAR(calendar));	
	//custom_calendar_update(CUSTOM_CALENDAR(calendar));
	
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
	//print_array(evt_arry_day);	
	display_event_array(evt_arry_day);	
	show_notifications(evt_arry_day);	
	g_array_free(evt_arry_day, FALSE); //clear the array 
	
	custom_calendar_update(CUSTOM_CALENDAR(calendar));
		
	if(m_talk && m_talk_at_startup) {
		speak_events();		
	}
	
		
	custom_calendar_update(CUSTOM_CALENDAR(calendar));

	gtk_window_present (GTK_WINDOW (window));    //use present not show with gtk4
			
}

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
