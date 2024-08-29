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
// GTK4 Speaking Calendar
// Author: Alan Crispin <crispinalan@gmail.com> 
// Date: August 2024
// use make file to compile
//====================================================================

#include <gtk/gtk.h>
#include "customcalendar.h"
#include "calendarevent.h"
#include "dbmanager.h"

#include <glib/gstdio.h>  //needed for g_mkdir
#include <math.h>  //compile with -lm
#include <libnotify/notify.h>

#include "dictionary.h"

#define CONFIG_DIRNAME "speaking-calendar"
#define CONFIG_FILENAME "speaking-calendar"
static char * m_config_file = NULL;

//Declarations

static void callbk_new_event(GSimpleAction *action, GVariant *parameter,  gpointer user_data);
static void callbk_add_new_event(GtkButton *button, gpointer user_data);

static void callbk_edit_event(GSimpleAction *action, GVariant *parameter,  gpointer user_data);
static void callbk_update_event(GtkButton *button, gpointer user_data);

static void callbk_delete_event(GtkButton *button, gpointer  user_data);
static void callbk_dropdown_summary(GtkDropDown* self, gpointer user_data); 
//import/export
static void callbk_import(GSimpleAction *action, GVariant *parameter,  gpointer user_data);
static void file_chooser_response (GObject *source, GAsyncResult *result, void *user_data);
gboolean import_ical_file(char* file_name, gpointer user_data);
static void callbk_export(GSimpleAction *action, GVariant *parameter,  gpointer user_data);
static void file_save_response (GObject *source, GAsyncResult *result, void *user_data);
void export_file(char *file_name);
gboolean file_exists(const char *file_name);

static void window_header(GtkWindow *window);

////Actions
static void callbk_about(GSimpleAction* action, GVariant *parameter, gpointer user_data);
static void callbk_info(GSimpleAction *action, GVariant *parameter,  gpointer user_data);

static void callbk_set_preferences(GtkButton *button, gpointer  user_data);
static void callbk_preferences(GSimpleAction* action, GVariant *parameter,gpointer user_data);

static void callbk_quit(GSimpleAction* action,G_GNUC_UNUSED GVariant *parameter, gpointer user_data);
static void callbk_delete_all(GSimpleAction *action, GVariant *parameter,  gpointer user_data);

static void callbk_export(GSimpleAction *action, GVariant *parameter,  gpointer user_data);
static void callbk_import(GSimpleAction *action, GVariant *parameter,  gpointer user_data);

//calendar callbks
static void callbk_calendar_next_month(CustomCalendar *calendar, gpointer user_data);
static void callbk_calendar_prev_month(CustomCalendar *calendar, gpointer user_data);
static void callbk_calendar_next_year(CustomCalendar *calendar, gpointer user_data);
static void callbk_calendar_prev_year(CustomCalendar *calendar, gpointer user_data);
static void callbk_calendar_home(GSimpleAction * action, GVariant *parameter, gpointer user_data);

static void set_titles_on_calendar(CustomCalendar *calendar);

char* get_public_holiday_str(int day); 
gboolean is_public_holiday(int day);
GDate* calculate_easter(gint year);
GList *get_public_holiday_speak_list();

static void callbk_spin_day_start(GtkSpinButton *button, gpointer user_data);
static void callbk_dropdown_month_start(GtkDropDown* self, gpointer user_data);
static void callbk_spin_year_start(GtkSpinButton *button, gpointer user_data);

static void callbk_spin_hour_start(GtkSpinButton *button, gpointer user_data);
static void callbk_spin_min_start(GtkSpinButton *button, gpointer user_data);
static void callbk_spin_hour_end(GtkSpinButton *button, gpointer user_data);
static void callbk_spin_min_end(GtkSpinButton *button, gpointer user_data);

static void callbk_spin_hour_reminder(GtkSpinButton *button, gpointer user_data);
static void callbk_spin_min_reminder(GtkSpinButton *button, gpointer user_data);

static void callbk_check_button_allday_toggled (GtkCheckButton *check_button, gpointer user_data);
static void callbk_check_button_upcoming_toggled(GtkCheckButton *check_button, gpointer user_data);

static void callbk_check_button_hasreminder_toggled(GtkCheckButton *check_button, gpointer user_data);
static void callbk_check_button_hasreminder_toggled2(GtkCheckButton *check_button, gpointer user_data);

gboolean is_colour_name(gchar* colour_name);
//Day selected
static void callbk_calendar_day_selected(CustomCalendar *calendar, gpointer user_data);
static void dialog_header_day_selected(GtkWindow *window);

static int get_month_number(const char* month_str);
static guint get_dropdown_position_month(const char* month);

static char *ignore_first_zero(char *input);
static char *remove_zeros(const char *text);
static char *remove_commas(const char *text);
static char *remove_fullstop(const char *text);
static char* remove_semicolons (const char *text);
static char* remove_question_marks (const char *text);
static char* remove_explanation_marks (const char *text);
static char* remove_punctuations(const char *text);
static char* replace_hypens(const char *text);
static char* replace_newlines(const char *text);

static int first_day_of_month(int month, int year);

char* get_time_str(int hour, int min);

//Search
static void callbk_search(GSimpleAction *action, GVariant *parameter,  gpointer user_data);
static void callbk_search_events(GtkButton *button, gpointer user_data);
static void search_events(const char* search_str);

//Speaking
static void speak_events();
static void callbk_speak(GSimpleAction* action, GVariant *parameter,gpointer user_data);
static void callbk_speaktime(GSimpleAction * action, GVariant *parameter, gpointer user_data);

static void callbk_reset_speaking(GSimpleAction *action, GVariant *parameter,  gpointer user_data);

static void task_callbk(GObject *gobject,GAsyncResult *res,  gpointer  user_data);

static guint get_dropdown_position_summary(const gchar* summary);

unsigned char *rawcat(unsigned char *arrys[], unsigned int arry_size[], int arry_count);
unsigned int get_merge_size(unsigned int sizes_arry[], int arry_size);

static void play_audio_async (GTask *task,
                          gpointer object,
                          gpointer task_data,
                          GCancellable *cancellable);

static void speak_time(gint hour, gint min);
static void speak_reminder();

static char* get_cardinal_string(int number);
static char* get_day_number_ordinal_string(int day);
static char* get_day_of_week(int day, int month, int year);
//static char* get_event_title_word(char* word);


GArray*  get_upcoming_array(int upcoming_days);
int  get_total_number_of_events();

static gboolean callbk_timer_update(gpointer data);

static void send_notification(gpointer  user_data);
//======================================================================

//selection
static int m_id_selection=-1;
static int m_index=-1; //day events listbox row index
CalendarEvent *selected_evt;

//calendar
static int m_12hour_format=1; //am pm hour format
static int m_show_end_time=0; //show end_time
static int m_holidays=0; // uk public holidays

static int m_show_tooltips=1; //TRUE

static int m_today_year=0;
static int m_today_month=0;
static int m_today_day=0;

//static const char* m_summary ="summary";
static const char* m_location ="";
static const char* m_description ="todo";

static int m_start_year=0;
static int m_start_month=0;
static int m_start_day=0;

//multiday TODO
static int m_end_year=0;
static int m_end_month=0;
static int m_end_day=0;

static int m_current_month=0;

static int m_start_hour=0;
static int m_start_min=0;
static int m_end_hour=0;
static int m_end_min=0;

static int m_has_reminder=0; 
static int m_reminder_hour=0;
static int m_reminder_min=0;

static gboolean continue_timer = TRUE;

static int m_priority=0;

static int m_is_yearly=0;
static int m_is_monthly=0;
static int m_is_weekly=0;
static int m_is_allday=0;

static  char* m_todaycolour="rgb(173,216,230)";
static  char* m_eventcolour="rgb(222,184,135)";
static char* m_holidaycolour="rgb(102,205,170)"; 

//Speaking
//speak preferences
static int m_speak =1;
static int m_speak_at_startup =0;
static int m_speak_upcoming=0;
static int m_speak_event_number=1;
static int m_speak_time=1;
static int m_speak_event_words=1; 
static int m_speak_rate=7000;
static int m_reset_preferences=0;

//static int m_speak_overlap=0;
static int m_speak_priority=0;

gboolean m_speaking=FALSE;
static gchar* m_raw_file ="/tmp/textout.raw";

static int m_upcoming_days=7;

//====================================================================

const GActionEntry app_actions[] = {
  { "speak", callbk_speak}, 
  { "speaktime", callbk_speaktime},   
  { "home", callbk_calendar_home}, 
  { "newevent", callbk_new_event},
  { "info", callbk_info},
  { "preferences", callbk_preferences},
  { "quit", callbk_quit}
};

//======================================================================

const char * const events[] = { 
	"Activity",	//0
	"Anniversary",	 //1
	"Appointment",//2
	"Birthday",	//3
	"Cafe",  //4
	"Car", //5
	"Delivery", //6
	"Dentist", //7
	"Driver", //8
	"Family", //9
	"Funeral",	//10
	"Holiday",//11
	"Hospital",	//12
	"Medical", //13
	"Meeting", //14
	"Meetup", //15
	"Memo",//16
	"Party",//17
	"Payment",//18
	"Project",	//19
	"Restaurant", //20
	"Sport", //21
	"Task", //22
	"Television", //23
	"Theatre",	//24	
	"Travel",  //25
	"Visit",//26
	"Work", //27 
	NULL };

const char* m_summary ="Activity";

//===========================================================
static guint get_dropdown_position_summary(const gchar* summary)
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
	if (g_strcmp0(summary_lower,"delivery")==0) {
	position=6;
	}
	if (g_strcmp0(summary_lower,"dentist")==0) {
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
	if (g_strcmp0(summary_lower,"holiday")==0) {
	position=11;
	}			
	if (g_strcmp0(summary_lower,"hospital")==0) {
	position=12;
	}
	if (g_strcmp0(summary_lower,"medical")==0) {
	position=13;
	}
	if (g_strcmp0(summary_lower,"meeting")==0) {
	position=14;
	}
	if (g_strcmp0(summary_lower,"meetup")==0) {
	position=15;
	}
	if (g_strcmp0(summary_lower,"memo")==0) {
	position=16;
	}
	if (g_strcmp0(summary_lower,"party")==0) {
	position=17;
	}	
	if (g_strcmp0(summary_lower,"payment")==0) {
	position=18;
	}	
	if (g_strcmp0(summary_lower,"project")==0) {
	position=19;
	}
	if (g_strcmp0(summary_lower,"restaurant")==0) {
	position=20;
	}
	if (g_strcmp0(summary_lower,"sport")==0) {
	position=21;
	}
	if (g_strcmp0(summary_lower,"task")==0) {
	position=22;
	}
	if (g_strcmp0(summary_lower,"television")==0) {
	position=23;
	}
	if (g_strcmp0(summary_lower,"theatre")==0) {
	position=24;
	}
	if (g_strcmp0(summary_lower,"travel")==0) {
	position=25;
	}	
	if (g_strcmp0(summary_lower,"visit")==0) {
	position=26;
	}	
	if (g_strcmp0(summary_lower,"work")==0) {
	position=27;
	}	
	return position;
}

//===================================================================

const char * const month_strs[] = { 
	"January",	
	"February",	
	"March",
	"April",	
	"May",  
	"June", 
	"July", 
	"August", 
	"September",
	"October", 
	"November",
	"December",		 
	NULL };


//===================================================================
//======================================================================
static guint get_dropdown_position_month(const char* month)
{
	
	guint position=0;//starts at zero not one
	gchar* summary_lower= g_ascii_strdown(month,-1);
		
	if (g_strcmp0(summary_lower,"january")==0) {
	position=0;
	}
	if (g_strcmp0(summary_lower,"february")==0) {
	position=1;
	}	
	if (g_strcmp0(summary_lower,"march")==0) {
	position=2;
	}
	if (g_strcmp0(summary_lower,"april")==0) {
	position=3;
	}
		if (g_strcmp0(summary_lower,"may")==0) {
	position=4;
	}
	if (g_strcmp0(summary_lower,"june")==0) {
	position=5;
	}
	if (g_strcmp0(summary_lower,"july")==0) {
	position=6;
	}
	if (g_strcmp0(summary_lower,"august")==0) {
	position=7;
	}
	if (g_strcmp0(summary_lower,"september")==0) {
	position=8;
	}
	if (g_strcmp0(summary_lower,"october")==0) {
	position=9;
	}	
	if (g_strcmp0(summary_lower,"november")==0) {
	position=10;
	}	
	if (g_strcmp0(summary_lower,"december")==0) {
	position=11;
	}
	
	return position;
}


//=====================================================================
char* get_time_str(int hour, int min)
{
	char *time_str = "";
	char *hour_str = "";
	char *min_str = "";	
	char *ampm_str = " ";
	
	if (m_12hour_format)
	{
	
	if (hour >= 13 && hour <= 23)
	{
	int shour = hour;
	shour = shour - 12;
	ampm_str = "pm ";
	hour_str = g_strdup_printf("%d", shour);
	}
	if(hour == 12)
	{
	ampm_str = "pm ";					
	hour_str = g_strdup_printf("%d", hour);
	 }
	if(hour <12)
	{
	ampm_str = "am ";					
	hour_str = g_strdup_printf("%d", hour);
	}	
	} // 12 hour format
		
	else //24 hour
	{
	hour_str = g_strdup_printf("%d", hour);
	} // 24
	
	min_str = g_strdup_printf("%d", min);
	
	if (min < 10)
	{
	time_str = g_strconcat(time_str, hour_str, ":0", min_str, NULL);
	}
	else
	{
	time_str = g_strconcat(time_str, hour_str, ":", min_str, NULL);
	}
	
	if (m_12hour_format) time_str = g_strconcat(time_str, ampm_str, NULL);
	else time_str = g_strconcat(time_str, " ", NULL);
	
	return time_str;
	
}

//======================================================================
// Background Timer
//======================================================================

static gboolean callbk_timer_update(gpointer data)
{
       
    GDateTime *date_time;
    //gchar *dt_format;
    date_time = g_date_time_new_now_local(); // get local time
    gint now_year = g_date_time_get_year(date_time);
	gint now_month =g_date_time_get_month(date_time);    
	gint now_day = g_date_time_get_day_of_month(date_time); 
	gint now_hour =g_date_time_get_hour(date_time);
	gint now_min =g_date_time_get_minute (date_time);
	gint sec = g_date_time_get_second (date_time);
	
	GArray *evt_arry_day; //get day events if any	
	evt_arry_day = g_array_new(FALSE, FALSE, sizeof(CALENDAR_TYPE_EVENT)); // setup arraylist
	db_get_all_events_year_month_day(evt_arry_day, now_year,now_month, now_day);
	//cycle through day event and check for reminders		
	for (int i = 0; i < evt_arry_day->len; i++)
	{
	CalendarEvent *evt_day = g_array_index(evt_arry_day , CalendarEvent *, i); //sorted
	
	int start_day=0;
	int start_month=0;
	int start_year=0;
	char* summary_str="";
	char* description_str="";
	char* location_str="";
	int start_hour=0;
	int start_min=0;
	int has_reminder=0;
	int reminder_hour=0;
	int reminder_min=0;
	int is_allday=0;
	
	g_object_get (evt_day, "startday", &start_day, NULL);
	g_object_get (evt_day, "startmonth", &start_month, NULL);
	g_object_get (evt_day, "startyear", &start_year, NULL);	
	g_object_get(evt_day, "summary", &summary_str, NULL);
	g_object_get(evt_day, "description", &description_str, NULL);
	g_object_get(evt_day, "location", &location_str, NULL);		
	g_object_get(evt_day, "starthour", &start_hour, NULL);
	g_object_get(evt_day, "startmin", &start_min, NULL);
	g_object_get(evt_day, "isallday", &is_allday, NULL);		
	
	g_object_get(evt_day, "hasreminder", &has_reminder, NULL);
	g_object_get(evt_day, "reminderhour", &reminder_hour, NULL);
	g_object_get(evt_day, "remindermin", &reminder_min, NULL);
		
	if(has_reminder ==1 && reminder_hour==now_hour && reminder_min==now_min && sec==0)
	{
				
		char* time_str ="";
		
		if(is_allday)
		{
			time_str ="All Day";
		}
		else
		{
			time_str =get_time_str(start_hour,start_min);
		}
						
		char *notify_str="";
				
		notify_str =g_strconcat(notify_str,
		time_str, " ",summary_str, " ",
		description_str, " ",location_str, NULL);
		 		
		send_notification((gpointer) notify_str);
		
		speak_reminder();		
	} //if reminder
    } //day_events
	  
    return continue_timer;
}


//=====================================================================

//====================================================================

static void send_notification(gpointer  user_data)
{			
	char* user_str =(gpointer) user_data;
		
	const char* title= "Calendar Reminder";
	const char* body=g_strdup(user_str);
	
	NotifyNotification *notify;
	notify_init("notification"); 
	notify = notify_notification_new(title, body, NULL);		
	notify_notification_set_urgency (notify, NOTIFY_URGENCY_NORMAL);
	notify_notification_show(notify,NULL);	 
	notify_uninit();
	
}
//=====================================================================

//======================================================================
// Save load config file
//======================================================================

static void config_load_default()
{
		
	//speaking	
	m_speak=1;
	m_speak_at_startup=0;
	m_speak_upcoming=0;
	m_speak_event_number=1;
	m_speak_event_words=1;
	m_speak_time=1;
	m_speak_rate=7000;
		
	//calendar
	m_12hour_format=1;
	m_show_end_time=0;
	m_holidays=0;
	m_show_tooltips=1;
	m_todaycolour="rgb(173,216,230)";
	m_eventcolour="rgb(222,184,135)";
	m_holidaycolour="rgb(102,205,170)";
}

//======================================================================

static void config_read()
{
	// Clean up previously loaded configuration values	
	//speaking	
	m_speak=1;
	m_speak_at_startup=0;
	m_speak_upcoming=0;
	m_speak_event_number=1;
	m_speak_event_words=1;
	m_speak_time=1;
	m_speak_rate=7000;		
	
	m_upcoming_days=7;
		
	//calendar
	m_12hour_format=1;
	m_show_end_time=0;
	m_holidays=0;
	m_show_tooltips=1;
	m_todaycolour="rgb(173,216,230)";
	m_eventcolour="rgb(222,184,135)";
	m_holidaycolour="rgb(102,205,170)";
	
	
	// Load keys from keyfile
	GKeyFile * kf = g_key_file_new();
	g_key_file_load_from_file(kf, m_config_file, G_KEY_FILE_NONE, NULL);

	//speaking	
	m_speak = g_key_file_get_integer(kf, "calendar_settings", "speak", NULL);
	m_speak_at_startup=g_key_file_get_integer(kf, "calendar_settings", "speak_startup", NULL);
	m_speak_upcoming=g_key_file_get_integer(kf, "calendar_settings", "speak_upcoming", NULL);
	m_speak_event_number=g_key_file_get_integer(kf, "calendar_settings", "speak_event_number", NULL);
	m_speak_event_words=g_key_file_get_integer(kf, "calendar_settings", "speak_event_words", NULL);
	m_speak_time=g_key_file_get_integer(kf, "calendar_settings", "speak_time", NULL);
	m_speak_rate=g_key_file_get_integer(kf, "calendar_settings", "speak_rate", NULL);
		
	m_upcoming_days=g_key_file_get_integer(kf, "calendar_settings", "upcoming_days", NULL);
		
	//calendar
	m_12hour_format=g_key_file_get_integer(kf, "calendar_settings", "hour_format", NULL);
	m_show_end_time = g_key_file_get_integer(kf, "calendar_settings", "show_end_time", NULL);
	m_holidays = g_key_file_get_integer(kf, "calendar_settings", "holidays", NULL);
	m_show_tooltips = g_key_file_get_integer(kf, "calendar_settings", "show_tooltips", NULL);
		
	m_todaycolour=g_key_file_get_string(kf, "calendar_settings", "todaycolour", NULL);
    m_eventcolour=g_key_file_get_string(kf, "calendar_settings", "eventcolour", NULL);
    m_holidaycolour=g_key_file_get_string(kf, "calendar_settings", "holidaycolour", NULL);
        	
	g_key_file_free(kf);
}
//======================================================================

void config_write()
{

	GKeyFile * kf = g_key_file_new();

	//speak	
	g_key_file_set_integer(kf, "calendar_settings", "speak", m_speak);
	g_key_file_set_integer(kf, "calendar_settings", "speak_startup", m_speak_at_startup);
	g_key_file_set_integer(kf, "calendar_settings", "speak_upcoming", m_speak_upcoming);
	g_key_file_set_integer(kf, "calendar_settings", "speak_event_number", m_speak_event_number);
	g_key_file_set_integer(kf, "calendar_settings", "speak_event_words", m_speak_event_words);
	g_key_file_set_integer(kf, "calendar_settings", "speak_time", m_speak_time);
	g_key_file_set_integer(kf, "calendar_settings", "speak_rate", m_speak_rate);
	
	g_key_file_set_integer(kf, "calendar_settings", "upcoming_days", m_upcoming_days);	
	
	
	//calendar
	g_key_file_set_integer(kf, "calendar_settings", "hour_format", m_12hour_format);
	g_key_file_set_integer(kf, "calendar_settings", "show_end_time", m_show_end_time);
	g_key_file_set_integer(kf, "calendar_settings", "holidays", m_holidays);
	g_key_file_set_integer(kf, "calendar_settings", "show_tooltips", m_show_tooltips);
	
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

//======================================================================
int  get_total_number_of_events(){

	return db_get_number_of_rows_all();
}
//======================================================================
static char *ignore_first_zero(char *input)
{    
  int len = strlen(input); 
  if(len > 0)
  {
    gunichar fc = g_utf8_get_char(input);
    if (fc == '0')
	{ 	
		input++;	
	} // if
  }
  
  return input;
}
//=====================================================================

//=====================================================================

static char *remove_zeros(const char *text)
{
	GString *str;
	const char *p;
	str = g_string_new("");
	p = text;
	while (*p)
	{
		gunichar cp = g_utf8_get_char(p);
		if (cp != '0')
		{ 
			g_string_append_unichar(str, *p);
		} // if
		++p;
	}
	return g_string_free(str, FALSE);
}
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
		{ 
			g_string_append_unichar(str, *p);
		} // if
		++p;
	}
	return g_string_free(str, FALSE);
}
//======================================================================
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
	if ( cp != ';' ){ 
	g_string_append_unichar (str, *p);
	}//if
	++p;
	}
	return g_string_free (str, FALSE);
}
//======================================================================
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
//======================================================================
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
	if ( cp != '\'' ){ //remove all apostrophes
	g_string_append_unichar (str, *p);
	}//if
	++p;
	}
	return g_string_free (str, FALSE);
}
//======================================================================
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
//======================================================================
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


//=====================================================================
static int get_month_number(const char* month_str) {
	
	char* month= g_ascii_strdown(month_str,-1);
	
	if(g_strcmp0(month,"january")==0) return 1;
	else if(g_strcmp0(month,"february")==0) return 2;
	else if(g_strcmp0(month,"march")==0) return 3;
	else if(g_strcmp0(month,"april")==0) return 4;
	else if(g_strcmp0(month,"may")==0) return 5;
	else if(g_strcmp0(month,"june")==0) return 6;
	else if(g_strcmp0(month,"july")==0) return 7;
	else if(g_strcmp0(month,"august")==0) return 8;
	else if(g_strcmp0(month,"september")==0) return 9;
	else if(g_strcmp0(month,"october")==0) return 10;
	else if(g_strcmp0(month,"november")==0) return 11;
	else if(g_strcmp0(month,"december")==0) return 12;
	else return 0;	
}

//======================================================================
static int first_day_of_month(int month, int year)
{
    if (month < 3) {
        month += 12;
        year--;
    }
    int century = year / 100;
    year = year % 100;
    return (((13 * (month + 1)) / 5) +
            (century / 4) + (5 * century) +
            year + (year / 4)) % 7;
}
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
//======================================================================
char* get_public_holiday_str(int day) 
{

// UK public holidays
// New Year's Day: 1 January (DONE)
// Good Friday: March or April  (DONE)
// Easter Monday: March or April (DONE)
// Early May: First Monday of May (TODO)
// Spring Bank Holiday: Last Monday of May (DONE)
// Summer Bank Holiday: Last Monday of August (DONE)
// Christmas Day: 25 December (DONE)
// Boxing day: 26 December (DONE)

   //char *public_holiday_str="";

	//markup public holidays
	if (m_start_month==1 && day ==1) {
	return "New Year";
	//public_holiday_str =g_strconcat(public_holiday_str," New Year ", NULL); 
	
	//return " New Year ";
	}

	if (m_start_month==12 && day==25) {
	//christmas day
	return "Christmas Day";	
	}

	if (m_start_month==12 && day==26) {
	//boxing day
	return "Boxing Day";	
	}

	if (m_start_month == 5) {
     //May complicated
     GDate *first_monday_may;
     first_monday_may = g_date_new_dmy(1, m_start_month, m_start_year);


     while (g_date_get_weekday(first_monday_may) != G_DATE_MONDAY)
       g_date_add_days(first_monday_may,1);

     int may_day = g_date_get_day(first_monday_may);

     if( day==may_day) 
     return "Public Holiday "; //may bank holiday
    
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
     return "Spring Bank";   //spring bank holiday

	} //m_start_month ==5 (May)
	
		
	GDate *easter_date =calculate_easter(m_start_year);
	int easter_day = g_date_get_day(easter_date);
	int easter_month =g_date_get_month(easter_date);

	if(m_start_month==easter_month && day == easter_day)
	{
	//easter day
	return "Easter Day";	
	}

	g_date_subtract_days(easter_date,2);
	int easter_friday = g_date_get_day(easter_date);
	int easter_friday_month =g_date_get_month(easter_date);

	if(m_start_month==easter_friday_month && day ==easter_friday)
	{
	//easter friday
	return "Easter Friday";	
	}

	g_date_add_days(easter_date,3);
	int easter_monday = g_date_get_day(easter_date); //easter monday
	int easter_monday_month =g_date_get_month(easter_date);

	if(m_start_month==easter_monday_month && day ==easter_monday)
	{
	//easter monday
	return "Easter Monday";	
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
     return "Public Holiday";   //august bank holiday   

    } //m_start_month==8

	return "";
	//return public_holiday_str;
}
//======================================================================

static void set_titles_on_calendar(CustomCalendar *calendar) 
{	
	//custom_calendar_mark_day(CUSTOM_CALENDAR(calendar), 1);//testing
	
	custom_calendar_initialise_eventday_array(calendar);
	custom_calendar_initialise_tooltip_array(calendar);
	custom_calendar_initialise_holiday_array(calendar);
	custom_calendar_reset_marks(CUSTOM_CALENDAR(calendar));	
	custom_calendar_reset_holidays(CUSTOM_CALENDAR(calendar));	
	
	guint8 month_days =g_date_get_days_in_month(m_start_month,m_start_year);	
		
	//cycle through month days and display any events
	for (int day=1; day<=month_days; day++)
	{
	GArray *evt_arry_day; //day events	
	evt_arry_day = g_array_new(FALSE, FALSE, sizeof(CALENDAR_TYPE_EVENT)); // setup arraylist
	db_get_all_events_year_month_day(evt_arry_day, m_start_year,m_start_month, day);
	
	//display
	for (int i = 0; i < evt_arry_day->len; i++)
	{
	CalendarEvent *evt_day = g_array_index(evt_arry_day , CalendarEvent *, i); //sorted
	
	int start_day=0;
	int start_month=0;
	int start_year=0;
	char* summary_str="";
	char* summary_str11="";	
	char *location_str="";
	char *description_str="";	
	int start_hour=0;
	int start_min=0;	
	int end_hour=0;
	int end_min=0;
	int is_yearly=0;
	int is_allday=0;
	int is_priority=0;
	
	g_object_get (evt_day, "startday", &start_day, NULL);
	g_object_get (evt_day, "startmonth", &start_month, NULL);
	g_object_get (evt_day, "startyear", &start_year, NULL);	
	g_object_get(evt_day, "summary", &summary_str, NULL);	
	g_object_get(evt_day, "location", &location_str, NULL);
	g_object_get(evt_day, "description", &description_str, NULL);	
	g_object_get(evt_day, "starthour", &start_hour, NULL);
	g_object_get(evt_day, "startmin", &start_min, NULL);	
	g_object_get(evt_day, "endhour", &end_hour, NULL);
	g_object_get(evt_day, "endmin", &end_min, NULL);
	g_object_get(evt_day, "isyearly", &is_yearly, NULL);
	g_object_get(evt_day, "isallday", &is_allday, NULL);
	g_object_get(evt_day, "ispriority", &is_priority, NULL);
	
	char *display_str="";
	char *tooltip_str="";
	char *time_str = "";
	char *starthour_str = "";
	char *startmin_str = "";
	char *endhour_str = "";
	char *endmin_str = "";
	char *ampm_str = " ";
	
	summary_str11 =g_strndup(summary_str,8);
	summary_str11 =g_strconcat(summary_str11,"...",NULL);	
	
	if(!is_allday)
	{		
	time_str =get_time_str(start_hour,start_min);
    display_str = g_strconcat(display_str, time_str, summary_str11, NULL);	
    //tooltip_str = g_strconcat(tooltip_str, time_str, summary_str, "\n",location_str, NULL);	
    tooltip_str = g_strconcat(tooltip_str, time_str, summary_str, "\n",description_str, "\n", location_str, NULL);	
   } //if !all_day	
   else
   {
	   display_str = g_strconcat(display_str, summary_str11, NULL);	
	   //tooltip_str = g_strconcat(tooltip_str, summary_str, "\n",location_str, NULL);
	   tooltip_str = g_strconcat(tooltip_str, summary_str, "\n",description_str, "\n", location_str, NULL);	
   }
	
	custom_calendar_set_day_str(CUSTOM_CALENDAR(calendar), start_day, display_str); 
	custom_calendar_set_tooltip_str(CUSTOM_CALENDAR(calendar), start_day, tooltip_str); 
	custom_calendar_mark_day(CUSTOM_CALENDAR(calendar), start_day);		
	
	} //for day	events
		
	} //for each day in month

	//markup public holidays
	if(m_holidays) {
	guint8 month_days =g_date_get_days_in_month(m_start_month,m_start_year);	
		
	for (int day=1; day<=month_days; day++)
	{
		if (is_public_holiday(day))
		{
			char *hol_str = get_public_holiday_str(day);
			custom_calendar_set_holiday_str(CUSTOM_CALENDAR(calendar), day, hol_str); 
			//custom_calendar_set_holiday_str(CUSTOM_CALENDAR(calendar), day, "Public Holiday"); 	
			custom_calendar_mark_holiday(CUSTOM_CALENDAR(calendar),day);
		}
	}	
    } //if m_holidys	
		
}

//=====================================================================
static void callbk_spin_day_start(GtkSpinButton *button, gpointer user_data)
{	
	m_start_day = gtk_spin_button_get_value_as_int (button);	
}
//=====================================================================
static void callbk_dropdown_month_start(GtkDropDown* self, gpointer user_data)
{		
	const char* month = gtk_string_object_get_string (GTK_STRING_OBJECT (gtk_drop_down_get_selected_item (self)));
	m_start_month=get_month_number(month);		
}
//=====================================================================
static void callbk_spin_year_start(GtkSpinButton *button, gpointer user_data)
{	
	m_start_year = gtk_spin_button_get_value_as_int (button);	
}
//======================================================================
static void callbk_spin_hour_start(GtkSpinButton *button, gpointer user_data)
{	
	m_start_hour = gtk_spin_button_get_value_as_int (button);	
}
//======================================================================
static void callbk_spin_min_start(GtkSpinButton *button, gpointer user_data)
{	
	m_start_min = gtk_spin_button_get_value_as_int (button);	
}
//======================================================================

static void callbk_spin_hour_end(GtkSpinButton *button, gpointer user_data)
{	
	m_end_hour = gtk_spin_button_get_value_as_int (button);
	//clamping checks
	if (m_end_hour<m_start_hour) m_end_hour =m_start_hour;	
}
//======================================================================

static void callbk_spin_min_end(GtkSpinButton *button, gpointer user_data)
{	
	m_end_min = gtk_spin_button_get_value_as_int (button);	
	//clamping check
	if ((m_end_hour == m_start_hour) && (m_end_min < m_start_min)) m_end_min =m_start_min;
}
//======================================================================
//======================================================================
static void callbk_spin_hour_reminder(GtkSpinButton *button, gpointer user_data)
{	
	m_reminder_hour = gtk_spin_button_get_value_as_int (button);		
}
//======================================================================
static void callbk_spin_min_reminder(GtkSpinButton *button, gpointer user_data)
{	
	m_reminder_min = gtk_spin_button_get_value_as_int (button);		
}
//======================================================================

static void callbk_check_button_hasreminder_toggled2(GtkCheckButton *check_button, gpointer user_data)
{
	GtkWidget *spin_button_reminder_hour;
	GtkWidget *spin_button_reminder_min;
	
	spin_button_reminder_hour = g_object_get_data(G_OBJECT(user_data), "cb_hasreminder_spin_reminder_hour_key");
	spin_button_reminder_min = g_object_get_data(G_OBJECT(user_data), "cb_hasreminder_spin_reminder_min_key");

	if (gtk_check_button_get_active(GTK_CHECK_BUTTON(check_button)))
	{
		gtk_widget_set_sensitive(spin_button_reminder_hour, FALSE);
		gtk_widget_set_sensitive(spin_button_reminder_min, FALSE);			

	}
	else
	{
		
		gtk_widget_set_sensitive(spin_button_reminder_hour, TRUE);
		gtk_widget_set_sensitive(spin_button_reminder_min, TRUE);
	}
					
	
}

//======================================================================
static void callbk_check_button_hasreminder_toggled(GtkCheckButton *check_button, gpointer user_data)
{
	GtkWidget *spin_button_reminder_hour;
	GtkWidget *spin_button_reminder_min;
	
	spin_button_reminder_hour = g_object_get_data(G_OBJECT(user_data), "cb_hasreminder_spin_reminder_hour_key");
	spin_button_reminder_min = g_object_get_data(G_OBJECT(user_data), "cb_hasreminder_spin_reminder_min_key");

	if (gtk_check_button_get_active(GTK_CHECK_BUTTON(check_button)))
	{
		gtk_widget_set_sensitive(spin_button_reminder_hour, TRUE);
		gtk_widget_set_sensitive(spin_button_reminder_min, TRUE);
	}
	else
	{
		gtk_widget_set_sensitive(spin_button_reminder_hour, FALSE);
		gtk_widget_set_sensitive(spin_button_reminder_min, FALSE);			
	}
					
	
}
//======================================================================
static void callbk_check_button_allday_toggled(GtkCheckButton *check_button, gpointer user_data)
{

	GtkWidget *spin_button_start_hour;
	GtkWidget *spin_button_start_min;
	GtkWidget *spin_button_end_hour;
	GtkWidget *spin_button_end_min;
	GtkWidget *check_button_reminder;
		
	spin_button_start_hour = g_object_get_data(G_OBJECT(user_data), "cb_allday_spin_start_hour_key");
	spin_button_start_min = g_object_get_data(G_OBJECT(user_data), "cb_allday_spin_start_min_key");
	spin_button_end_hour = g_object_get_data(G_OBJECT(user_data), "cb_allday_spin_end_hour_key");
	spin_button_end_min = g_object_get_data(G_OBJECT(user_data), "cb_allday_spin_end_min_key");
	
	

	if (gtk_check_button_get_active(GTK_CHECK_BUTTON(check_button)))
	{
		gtk_widget_set_sensitive(spin_button_start_hour, FALSE);
		gtk_widget_set_sensitive(spin_button_start_min, FALSE);
		gtk_widget_set_sensitive(spin_button_end_hour, FALSE);
		gtk_widget_set_sensitive(spin_button_end_min, FALSE);		
	}
	else
	{
		gtk_widget_set_sensitive(spin_button_start_hour, TRUE);
		gtk_widget_set_sensitive(spin_button_start_min, TRUE);
		gtk_widget_set_sensitive(spin_button_end_hour, TRUE);
		gtk_widget_set_sensitive(spin_button_end_min, TRUE);
	}
}
//======================================================================
static void callbk_dropdown_summary(GtkDropDown* self, gpointer user_data)
{	
	m_summary = gtk_string_object_get_string (GTK_STRING_OBJECT (gtk_drop_down_get_selected_item (self)));	
}
//======================================================================

static void callbk_add_new_event(GtkButton *button, gpointer user_data)
{

	g_return_if_fail(GTK_IS_BUTTON(button));
	
	GtkWidget *window = user_data;
	GtkWidget *calendar = g_object_get_data(G_OBJECT(window), "window-calendar-key");
	GtkWidget *dialog = g_object_get_data(G_OBJECT(button), "dialog-key");
	
	GtkEntryBuffer *buffer_summary;
	GtkWidget *entry_summary = g_object_get_data(G_OBJECT(button), "entry-summary-key");
	
	GtkEntryBuffer *buffer_location;
	GtkWidget *entry_location = g_object_get_data(G_OBJECT(button), "entry-location-key");
	
	GtkEntryBuffer *buffer_description;
	GtkWidget *entry_description = g_object_get_data(G_OBJECT(button), "entry-description-key");
	
	GtkWidget *check_button_allday = g_object_get_data(G_OBJECT(button), "check-button-allday-key");
	GtkWidget *check_button_multiday = g_object_get_data(G_OBJECT(button), "check-button-multiday-key");
	GtkWidget *check_button_isyearly = g_object_get_data(G_OBJECT(button), "check-button-isyearly-key");
	GtkWidget *check_button_priority = g_object_get_data(G_OBJECT(button), "check-button-priority-key");
	
	GtkWidget *spin_button_day_start = g_object_get_data(G_OBJECT(button), "spin-day-start-key");
	GtkWidget *spin_button_year_start= g_object_get_data(G_OBJECT(button), "spin-year-start-key");
	
	GtkWidget *spin_button_start_hour = g_object_get_data(G_OBJECT(button), "spin-start-hour-key");
	GtkWidget *spin_button_start_min = g_object_get_data(G_OBJECT(button), "spin-start-min-key");
	
	GtkWidget *spin_button_end_hour = g_object_get_data(G_OBJECT(button), "spin-end-hour-key");
	GtkWidget *spin_button_end_min = g_object_get_data(G_OBJECT(button), "spin-end-min-key");
	
	//reminder	
	GtkWidget *check_button_hasreminder = g_object_get_data(G_OBJECT(button), "check-button-hasreminder-key");
	GtkWidget *spin_button_reminder_hour = g_object_get_data(G_OBJECT(button), "spin-reminder-hour-key");
	GtkWidget *spin_button_reminder_min = g_object_get_data(G_OBJECT(button), "spin-reminder-min-key");
	
	//m_summary set by dropdown
	
	m_description="";		
	buffer_description = gtk_entry_get_buffer(GTK_ENTRY(entry_description));
	m_description = gtk_entry_buffer_get_text(buffer_description);
	
	m_description = remove_semicolons(m_description);
	m_description = remove_commas(m_description);
	m_description =remove_punctuations(m_description);
	
	m_location="";
	buffer_location = gtk_entry_get_buffer(GTK_ENTRY(entry_location));
	m_location = gtk_entry_buffer_get_text(buffer_location);
	
	m_location = remove_semicolons(m_location);
	m_location = remove_commas(m_location);
	m_location =remove_punctuations(m_location);
	
	//capture typed values
	m_start_day= gtk_spin_button_get_value(GTK_SPIN_BUTTON(spin_button_day_start));
	m_start_year= gtk_spin_button_get_value(GTK_SPIN_BUTTON(spin_button_year_start));
	
	//m_end_day= gtk_spin_button_get_value(GTK_SPIN_BUTTON(spin_button_day_end));
	//m_end_year= gtk_spin_button_get_value(GTK_SPIN_BUTTON(spin_button_year_end));
	
	m_start_hour= gtk_spin_button_get_value(GTK_SPIN_BUTTON(spin_button_start_hour));
	m_start_min= gtk_spin_button_get_value(GTK_SPIN_BUTTON(spin_button_start_min));	
	m_end_hour= gtk_spin_button_get_value(GTK_SPIN_BUTTON(spin_button_end_hour));
	m_end_min= gtk_spin_button_get_value(GTK_SPIN_BUTTON(spin_button_end_min));
	
	m_is_allday = gtk_check_button_get_active(GTK_CHECK_BUTTON(check_button_allday));
	
	if (m_is_allday) 
	{
		m_start_hour=0; //sorting to top
		m_start_min=0;
		m_end_hour=0;
		m_end_min=0;
	 }
	
	
	m_is_yearly = gtk_check_button_get_active(GTK_CHECK_BUTTON(check_button_isyearly));	
	m_priority = gtk_check_button_get_active(GTK_CHECK_BUTTON(check_button_priority));
	
	//TODO multiday
	m_end_day=m_start_day;
	m_end_month=m_start_month;
	m_end_year=m_start_year;
	
	m_has_reminder=0;
	m_reminder_hour=0;
	m_reminder_min =0;
	
	m_has_reminder = gtk_check_button_get_active(GTK_CHECK_BUTTON(check_button_hasreminder));
	m_reminder_hour= gtk_spin_button_get_value(GTK_SPIN_BUTTON(spin_button_reminder_hour));
	m_reminder_min= gtk_spin_button_get_value(GTK_SPIN_BUTTON(spin_button_reminder_min));
	
	//add event into db
		
	CalendarEvent *evt = g_object_new(CALENDAR_TYPE_EVENT, 0);
	
	g_object_set(evt, "summary", g_strdup(m_summary), NULL);
	g_object_set(evt, "location", g_strdup(m_location), NULL);
	g_object_set(evt, "description", g_strdup(m_description), NULL);
	g_object_set(evt, "startyear", m_start_year, NULL);
	g_object_set(evt, "startmonth", m_start_month, NULL);
	g_object_set(evt, "startday", m_start_day, NULL);
	g_object_set(evt, "starthour", m_start_hour, NULL);
	g_object_set(evt, "startmin", m_start_min, NULL);
	g_object_set(evt, "endyear", m_end_year, NULL); // to do
	g_object_set(evt, "endmonth", m_end_month, NULL);
	g_object_set(evt, "endday", m_end_day, NULL);
	g_object_set(evt, "endhour", m_end_hour, NULL);
	g_object_set(evt, "endmin", m_end_min, NULL);
	g_object_set(evt, "isyearly", m_is_yearly, NULL);
	g_object_set(evt, "isallday", m_is_allday, NULL);			
	g_object_set(evt, "ispriority", m_priority, NULL);	
	g_object_set(evt, "hasreminder", m_has_reminder, NULL);
	g_object_set(evt, "reminderhour", m_reminder_hour, NULL);
	g_object_set(evt, "remindermin", m_reminder_min, NULL);		
	db_insert_event(evt); //insert event into database	
	
	m_id_selection = -1;
	m_index=-1;
			
	if(m_current_month==m_start_month) {			
	  set_titles_on_calendar(CUSTOM_CALENDAR(calendar));
	}
	//set_titles_on_calendar(CUSTOM_CALENDAR(calendar));			
	custom_calendar_update(CUSTOM_CALENDAR(calendar));
	
	
	gtk_window_destroy(GTK_WINDOW(dialog));
	
}
//======================================================================

static void callbk_new_event(GSimpleAction *action, GVariant *parameter,  gpointer user_data)
{
		
	GtkWidget *window = user_data;
	GtkWidget *dialog;
	
	dialog = gtk_window_new(); 
	gtk_window_set_title(GTK_WINDOW(dialog), "New Event");
	
	GtkWidget *button_add_event;
	
	GtkWidget *grid;
	
	GtkWidget *label_summary;	
	GtkWidget *dropdown_summary;
	
	GtkWidget *label_description;
	GtkWidget *entry_description;
	
	GtkWidget *label_location;
	GtkWidget *entry_location;
	
	GtkWidget *label_spacer1;
	GtkWidget *label_spacer2;
	GtkWidget *label_spacer3;
	GtkWidget *label_spacer4;
	GtkWidget *label_spacer5;
	GtkWidget *label_spacer6;
	
	//date
	GtkWidget *label_date_start;
	GtkWidget *spin_button_day_start;	
	GtkWidget *dropdown_month_start;	
	GtkWidget *spin_button_year_start;
	
	//mulitday requires end date
	GtkWidget *label_end_date;
	GtkWidget *spin_button_end_day;	
	GtkWidget *dropdown_month_end;
	GtkWidget *spin_button_end_year;
	
	// Check buttons
	GtkWidget *check_button_allday;	
	GtkWidget *check_button_isyearly;
	GtkWidget *check_button_priority;
	GtkWidget *check_button_hasreminder;

	//Adjustments
	// value,lower,upper,step_increment,page_increment,page_size
	GtkAdjustment *adjustment_day = gtk_adjustment_new(1.00, 0.0, 31.00, 1.0, 1.0, 0.0);	
	GtkAdjustment *adjustment_year = gtk_adjustment_new(2024.00, 0.0, 5000.00, 1.0, 1.0, 0.0);
	GtkAdjustment *adjustment_day_end = gtk_adjustment_new(1.00, 0.0, 31.00, 1.0, 1.0, 0.0);
	GtkAdjustment *adjustment_year_end = gtk_adjustment_new(2024.00, 0.0, 5000.00, 1.0, 1.0, 0.0);
	
	//start time
	GtkWidget *label_start_time;
	GtkWidget *spin_button_start_hour;	
	GtkWidget *spin_button_start_min;
	//end time
	GtkWidget *label_end_time;
	GtkWidget *spin_button_end_hour;	
	GtkWidget *spin_button_end_min;
	
	//reminder time
	GtkWidget *label_reminder_time;	
	GtkWidget *spin_button_reminder_hour;	
	GtkWidget *spin_button_reminder_min;	
	
	GtkAdjustment *adjustment_start_hour = gtk_adjustment_new(1.00, 0.0, 23.00, 1.0, 1.0, 0.0);
	GtkAdjustment *adjustment_start_min= gtk_adjustment_new(1.00, 0.0, 59.00, 1.0, 1.0, 0.0);
	
	GtkAdjustment *adjustment_end_hour = gtk_adjustment_new(1.00, 0.0, 23.00, 1.0, 1.0, 0.0);
	GtkAdjustment *adjustment_end_min = gtk_adjustment_new(1.00, 0.0, 59.00, 1.0, 1.0, 0.0);
	
	GtkAdjustment *adjustment_reminder_hour = gtk_adjustment_new(1.00, 0.0, 23.00, 1.0, 1.0, 0.0);
	GtkAdjustment *adjustment_reminder_min= gtk_adjustment_new(1.00, 0.0, 59.00, 1.0, 1.0, 0.0);
	
	label_spacer1 = gtk_label_new("");
	label_spacer2 = gtk_label_new("");
	label_spacer3 = gtk_label_new("");
	label_spacer4 = gtk_label_new("");
	label_spacer5 = gtk_label_new("");
	label_spacer6 = gtk_label_new("");
	
	button_add_event = gtk_button_new_with_label ("Add Event");
	g_signal_connect (GTK_BUTTON (button_add_event),"clicked", G_CALLBACK (callbk_add_new_event), G_OBJECT (window));
	
	grid = gtk_grid_new();	
	gtk_grid_set_column_homogeneous(GTK_GRID(grid), TRUE);
	
	m_current_month=m_start_month;
	
	m_end_day=m_start_day;
	m_end_month=m_start_month;
	m_end_year=m_start_year;
	
	m_start_hour=0;
	m_start_min=0;
	m_end_hour=0;
	m_end_min=0;
	
	//Summary
		
	label_summary = gtk_label_new("Event Speech Word: ");	
	dropdown_summary =gtk_drop_down_new_from_strings(events);    
    g_signal_connect(GTK_DROP_DOWN(dropdown_summary), "notify::selected", G_CALLBACK(callbk_dropdown_summary), NULL);
	
	//description
	label_description = gtk_label_new("Description: ");
	entry_description = gtk_entry_new();
	gtk_entry_set_has_frame(GTK_ENTRY(entry_description),TRUE); 
	gtk_entry_set_max_length(GTK_ENTRY(entry_description), 100);
	
	//location
	label_location = gtk_label_new("Location: ");
	entry_location = gtk_entry_new();
	gtk_entry_set_has_frame(GTK_ENTRY(entry_location),TRUE); 
	gtk_entry_set_max_length(GTK_ENTRY(entry_location), 25);
		
	//start date
	label_date_start =gtk_label_new("Start Date: ");
	
	spin_button_day_start = gtk_spin_button_new(adjustment_day, 1.0, 0);	
	g_signal_connect(GTK_SPIN_BUTTON(spin_button_day_start), "value_changed", G_CALLBACK(callbk_spin_day_start), NULL);	
	gtk_spin_button_set_value(GTK_SPIN_BUTTON(spin_button_day_start), m_start_day);
	
	dropdown_month_start =gtk_drop_down_new_from_strings(month_strs);    
    g_signal_connect(GTK_DROP_DOWN(dropdown_month_start), "notify::selected", G_CALLBACK(callbk_dropdown_month_start), NULL);
    gtk_drop_down_set_selected(GTK_DROP_DOWN(dropdown_month_start),m_start_month-1);
	
	spin_button_year_start = gtk_spin_button_new(adjustment_year, 1.0, 0);
	g_signal_connect(GTK_SPIN_BUTTON(spin_button_year_start), "value_changed", G_CALLBACK(callbk_spin_year_start), NULL);		
	gtk_spin_button_set_value(GTK_SPIN_BUTTON(spin_button_year_start), m_start_year);
		
	//Times
	//start time
	label_start_time =gtk_label_new("Start Time: ");	
	
	spin_button_start_hour = gtk_spin_button_new(adjustment_start_hour, 1.0, 0);
	gtk_spin_button_set_value(GTK_SPIN_BUTTON(spin_button_start_hour), m_start_hour);
	g_signal_connect(GTK_SPIN_BUTTON(spin_button_start_hour), "value_changed", G_CALLBACK(callbk_spin_hour_start), NULL);	
		
	spin_button_start_min = gtk_spin_button_new(adjustment_start_min, 1.0, 0);
	gtk_spin_button_set_value(GTK_SPIN_BUTTON(spin_button_start_min), m_start_min);	
	g_signal_connect(GTK_SPIN_BUTTON(spin_button_start_min), "value_changed", G_CALLBACK(callbk_spin_min_start), NULL);
		
	//end time
	label_end_time =gtk_label_new("End Time: ");	
	
	spin_button_end_hour = gtk_spin_button_new(adjustment_end_hour, 1.0, 0);
	gtk_spin_button_set_value(GTK_SPIN_BUTTON(spin_button_end_hour), m_end_hour);
	g_signal_connect(GTK_SPIN_BUTTON(spin_button_end_hour), "value_changed", G_CALLBACK(callbk_spin_hour_end), NULL);
	
	spin_button_end_min = gtk_spin_button_new(adjustment_end_min, 1.0, 0);
	gtk_spin_button_set_value(GTK_SPIN_BUTTON(spin_button_end_min), m_end_min);
	g_signal_connect(GTK_SPIN_BUTTON(spin_button_end_min), "value_changed", G_CALLBACK(callbk_spin_min_end), NULL);
	
	//reminder time
	spin_button_reminder_hour = gtk_spin_button_new(adjustment_reminder_hour, 1.0, 0);	
	g_signal_connect(GTK_SPIN_BUTTON(spin_button_reminder_hour), "value_changed", G_CALLBACK(callbk_spin_hour_reminder), NULL);	
	gtk_spin_button_set_value(GTK_SPIN_BUTTON(spin_button_reminder_hour), m_reminder_hour);
	
	spin_button_reminder_min = gtk_spin_button_new(adjustment_reminder_min, 1.0, 0);		
	g_signal_connect(GTK_SPIN_BUTTON(spin_button_reminder_min), "value_changed", G_CALLBACK(callbk_spin_min_reminder), NULL);	
	gtk_spin_button_set_value(GTK_SPIN_BUTTON(spin_button_reminder_min), m_reminder_min);	
	
	label_reminder_time =gtk_label_new("Reminder Time:");
	
	check_button_hasreminder=gtk_check_button_new_with_label("Has Reminder");
	
	
	gtk_widget_set_sensitive(spin_button_reminder_hour, FALSE);
	gtk_widget_set_sensitive(spin_button_reminder_min, FALSE);
	
	//check button hasreminder
	g_signal_connect_swapped(GTK_CHECK_BUTTON(check_button_hasreminder), "toggled",
							 G_CALLBACK(callbk_check_button_hasreminder_toggled), check_button_hasreminder);
	
	g_object_set_data(G_OBJECT(check_button_hasreminder), "cb_hasreminder_spin_reminder_hour_key", spin_button_reminder_hour);
	g_object_set_data(G_OBJECT(check_button_hasreminder), "cb_hasreminder_spin_reminder_min_key", spin_button_reminder_min);						 
	
	
	// check button allday
	check_button_allday = gtk_check_button_new_with_label("Is All Day");
	g_signal_connect_swapped(GTK_CHECK_BUTTON(check_button_allday), "toggled",
							 G_CALLBACK(callbk_check_button_allday_toggled), check_button_allday);
	g_object_set_data(G_OBJECT(check_button_allday), "cb_allday_spin_start_hour_key", spin_button_start_hour);
	g_object_set_data(G_OBJECT(check_button_allday), "cb_allday_spin_start_min_key", spin_button_start_min);
	g_object_set_data(G_OBJECT(check_button_allday), "cb_allday_spin_end_hour_key", spin_button_end_hour);
	g_object_set_data(G_OBJECT(check_button_allday), "cb_allday_spin_end_min_key", spin_button_end_min);
		
	check_button_isyearly = gtk_check_button_new_with_label("Is Yearly");
	check_button_priority = gtk_check_button_new_with_label("Is High Priority");
	
	g_object_set_data(G_OBJECT(button_add_event), "dialog-key", dialog);

	g_object_set_data(G_OBJECT(button_add_event), "entry-location-key", entry_location);
	
	g_object_set_data(G_OBJECT(button_add_event), "entry-description-key", entry_description);
	
	//need to capture spin button values in case these are typed
	
	g_object_set_data(G_OBJECT(button_add_event), "spin-start-hour-key", spin_button_start_hour);
	g_object_set_data(G_OBJECT(button_add_event), "spin-start-min-key", spin_button_start_min);
	g_object_set_data(G_OBJECT(button_add_event), "spin-end-hour-key", spin_button_end_hour);
	g_object_set_data(G_OBJECT(button_add_event), "spin-end-min-key", spin_button_end_min);
	
	//reminder
	g_object_set_data(G_OBJECT(button_add_event), "check-button-hasreminder-key", check_button_hasreminder);
	g_object_set_data(G_OBJECT(button_add_event), "spin-reminder-hour-key", spin_button_reminder_hour);
	g_object_set_data(G_OBJECT(button_add_event), "spin-reminder-min-key", spin_button_reminder_min);
		
	g_object_set_data(G_OBJECT(button_add_event), "spin-day-start-key", spin_button_day_start);	
	g_object_set_data(G_OBJECT(button_add_event), "spin-year-start-key", spin_button_year_start);	
	g_object_set_data(G_OBJECT(button_add_event), "spin-day-end-key", spin_button_end_day);	
	g_object_set_data(G_OBJECT(button_add_event), "spin-year-end-key", spin_button_end_year);
		
	
	g_object_set_data(G_OBJECT(button_add_event), "check-button-allday-key", check_button_allday);	
	g_object_set_data(G_OBJECT(button_add_event), "check-button-isyearly-key", check_button_isyearly);
	g_object_set_data(G_OBJECT(button_add_event), "check-button-priority-key", check_button_priority);
		
	//gtk_grid_attach (GtkGrid* grid,  GtkWidget* child, int column,  int row,  int width, int height)

	gtk_grid_attach(GTK_GRID(grid), label_summary, 1, 1, 1, 1);
	gtk_grid_attach(GTK_GRID(grid), dropdown_summary, 2, 1, 1, 1);
	
	gtk_grid_attach(GTK_GRID(grid), label_description, 1, 2, 1, 1);
	gtk_grid_attach(GTK_GRID(grid), entry_description, 2, 2, 3, 1);
	
	gtk_grid_attach(GTK_GRID(grid), label_location, 1, 3, 1, 1);
	gtk_grid_attach(GTK_GRID(grid), entry_location, 2, 3, 3, 1);
	
	gtk_grid_attach(GTK_GRID(grid), label_spacer1,       1, 4, 3, 1);
	//start date
	gtk_grid_attach(GTK_GRID(grid), label_date_start,       1, 5, 1, 1);
	gtk_grid_attach(GTK_GRID(grid), spin_button_day_start,  2, 5, 1, 1);
	gtk_grid_attach(GTK_GRID(grid), dropdown_month_start,    3, 5, 1, 1);
	gtk_grid_attach(GTK_GRID(grid), spin_button_year_start,  4, 5, 1, 1);
		
	gtk_grid_attach(GTK_GRID(grid), label_spacer2,       1, 6, 3, 1);
	
	//start time
	gtk_grid_attach(GTK_GRID(grid), label_start_time,       1, 7, 1, 1);
	gtk_grid_attach(GTK_GRID(grid), spin_button_start_hour,  2, 7, 1, 1);
	gtk_grid_attach(GTK_GRID(grid), spin_button_start_min,   3, 7, 1, 1);
	//end time
	gtk_grid_attach(GTK_GRID(grid), label_end_time,        1, 8, 1, 1);
	gtk_grid_attach(GTK_GRID(grid), spin_button_end_hour,  2, 8, 1, 1);
	gtk_grid_attach(GTK_GRID(grid), spin_button_end_min,   3, 8, 1, 1);
	
	gtk_grid_attach(GTK_GRID(grid), label_spacer3,       1, 9, 3, 1);
	
	//reminder time
	gtk_grid_attach(GTK_GRID(grid), check_button_hasreminder,       1, 10, 1, 1);
	gtk_grid_attach(GTK_GRID(grid), label_reminder_time,            2, 10, 1, 1);
	gtk_grid_attach(GTK_GRID(grid), spin_button_reminder_hour,      3, 10, 1, 1);
	gtk_grid_attach(GTK_GRID(grid), spin_button_reminder_min,        4, 10, 1, 1);
	
	gtk_grid_attach(GTK_GRID(grid), label_spacer4,       1, 11, 3, 1); 
	
	gtk_grid_attach(GTK_GRID(grid), check_button_allday,        1, 12, 1, 1);
    gtk_grid_attach(GTK_GRID(grid), check_button_isyearly,      2, 12, 1, 1);  
    gtk_grid_attach(GTK_GRID(grid), check_button_priority,      3, 12, 1, 1);
   
    gtk_grid_attach(GTK_GRID(grid), label_spacer5,       1, 13, 3, 1);

	gtk_grid_attach(GTK_GRID(grid), button_add_event,  1, 14, 4, 1);

    gtk_window_set_child (GTK_WINDOW (dialog), grid);	
	gtk_window_present(GTK_WINDOW(dialog));	
	    
}

//======================================================================
// next month
//======================================================================
static void callbk_calendar_next_month(CustomCalendar *calendar, gpointer user_data) 
{
	m_start_day = custom_calendar_get_day(CUSTOM_CALENDAR(calendar));
	m_start_month = custom_calendar_get_month(CUSTOM_CALENDAR(calendar));
	m_start_year = custom_calendar_get_year(CUSTOM_CALENDAR(calendar));		
	
	//mark up calendar
	set_titles_on_calendar(CUSTOM_CALENDAR(calendar));
	custom_calendar_update(CUSTOM_CALENDAR(calendar));
		
}
//======================================================================
// Previous month
//======================================================================

static void callbk_calendar_prev_month(CustomCalendar *calendar, gpointer user_data) 
{	
	m_start_day = custom_calendar_get_day(CUSTOM_CALENDAR(calendar));
	m_start_month = custom_calendar_get_month(CUSTOM_CALENDAR(calendar));
	m_start_year = custom_calendar_get_year(CUSTOM_CALENDAR(calendar));	
		
	//mark up calendar	
	set_titles_on_calendar(CUSTOM_CALENDAR(calendar));
	custom_calendar_update(CUSTOM_CALENDAR(calendar));
		
}
//======================================================================
// Next year
//======================================================================

static void callbk_calendar_next_year(CustomCalendar *calendar, gpointer user_data) 
{
	m_start_day = custom_calendar_get_day(CUSTOM_CALENDAR(calendar));
	m_start_month = custom_calendar_get_month(CUSTOM_CALENDAR(calendar));
	m_start_year = custom_calendar_get_year(CUSTOM_CALENDAR(calendar));	
	
	//mark up calendar
	set_titles_on_calendar(CUSTOM_CALENDAR(calendar));
	custom_calendar_update(CUSTOM_CALENDAR(calendar));	
	
}
//======================================================================
// Previous year
//======================================================================

static void callbk_calendar_prev_year(CustomCalendar *calendar, gpointer user_data) 
{
	m_start_day = custom_calendar_get_day(CUSTOM_CALENDAR(calendar));
	m_start_month = custom_calendar_get_month(CUSTOM_CALENDAR(calendar));
	m_start_year = custom_calendar_get_year(CUSTOM_CALENDAR(calendar));	
	
	//mark up calendar
	set_titles_on_calendar(CUSTOM_CALENDAR(calendar));
	custom_calendar_update(CUSTOM_CALENDAR(calendar));		
}
//======================================================================


static void callbk_day_events_dialog(GtkButton *button, gpointer  user_data)
{	
	GtkWindow *window =user_data;	
	
	GtkWidget *dialog = g_object_get_data(G_OBJECT(button), "dialog-key");
	
	gtk_window_destroy(GTK_WINDOW(dialog));

}

//======================================================================

static void callbk_row_activated (GtkListBox    *listbox,
                                  GtkListBoxRow *row,
                                  gpointer       user_data){
  
  
	  m_index = gtk_list_box_row_get_index (row);
	  
	  GArray *evt_arry_day; //day events	
	  evt_arry_day = g_array_new(FALSE, FALSE, sizeof(CALENDAR_TYPE_EVENT)); // setup arraylist
	  db_get_all_events_year_month_day(evt_arry_day, m_start_year,m_start_month, m_start_day);
	  
	  selected_evt = g_array_index(evt_arry_day , CalendarEvent *, m_index);
	  
	  int db_id;	//this is the database id
	  char* summary_str;
	  int start_day=0;
	  int start_month=0;
	  int start_year=0;
	  
	  g_object_get (selected_evt, "eventid", &db_id, NULL);
	  g_object_get (selected_evt, "startday", &start_day, NULL);
	  g_object_get (selected_evt, "startmonth", &start_month, NULL);
	  g_object_get (selected_evt, "startyear", &start_year, NULL);	
	  g_object_get(selected_evt, "summary", &summary_str, NULL);
	  
	  m_id_selection=db_id;   
	  g_array_free(evt_arry_day, FALSE); //clear standard month events
	  
}

//======================================================================

static void callbk_update_event(GtkButton *button, gpointer user_data)
{
	GtkWidget *window = user_data;
	GtkWidget *calendar = g_object_get_data(G_OBJECT(window), "window-calendar-key");
	GtkWidget *dialog = g_object_get_data(G_OBJECT(button), "dialog-key");
	
	GtkEntryBuffer *buffer_location;
	GtkWidget *entry_location = g_object_get_data(G_OBJECT(button), "entry-location-key");

	GtkEntryBuffer *buffer_description;
	GtkWidget *entry_description = g_object_get_data(G_OBJECT(button), "entry-description-key");

	GtkWidget *check_button_allday = g_object_get_data(G_OBJECT(button), "check-button-allday-key");
	GtkWidget *check_button_multiday = g_object_get_data(G_OBJECT(button), "check-button-multiday-key");
	GtkWidget *check_button_isyearly = g_object_get_data(G_OBJECT(button), "check-button-isyearly-key");
	GtkWidget *check_button_priority = g_object_get_data(G_OBJECT(button), "check-button-priority-key");
	
	GtkWidget *spin_button_day_start = g_object_get_data(G_OBJECT(button), "spin-day-start-key");
	GtkWidget *spin_button_year_start= g_object_get_data(G_OBJECT(button), "spin-year-start-key");
	
	GtkWidget *spin_button_start_hour = g_object_get_data(G_OBJECT(button), "spin-start-hour-key");
	GtkWidget *spin_button_start_min = g_object_get_data(G_OBJECT(button), "spin-start-min-key");
	GtkWidget *spin_button_end_hour = g_object_get_data(G_OBJECT(button), "spin-end-hour-key");
	GtkWidget *spin_button_end_min = g_object_get_data(G_OBJECT(button), "spin-end-min-key");
	
	//reminder	
	GtkWidget *check_button_hasreminder = g_object_get_data(G_OBJECT(button), "check-button-hasreminder-key");
	GtkWidget *spin_button_reminder_hour = g_object_get_data(G_OBJECT(button), "spin-reminder-hour-key");
	GtkWidget *spin_button_reminder_min = g_object_get_data(G_OBJECT(button), "spin-reminder-min-key");
	
	//m_summary set by dropdown
		
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
		
	m_is_allday = gtk_check_button_get_active(GTK_CHECK_BUTTON(check_button_allday));	
	m_is_yearly = gtk_check_button_get_active(GTK_CHECK_BUTTON(check_button_isyearly));	
	m_priority = gtk_check_button_get_active(GTK_CHECK_BUTTON(check_button_priority));
			
	//capture typed values
	 m_start_day= gtk_spin_button_get_value(GTK_SPIN_BUTTON(spin_button_day_start));
	 m_start_year= gtk_spin_button_get_value(GTK_SPIN_BUTTON(spin_button_year_start));
			 
	 m_start_hour= gtk_spin_button_get_value(GTK_SPIN_BUTTON(spin_button_start_hour));
	 m_start_min= gtk_spin_button_get_value(GTK_SPIN_BUTTON(spin_button_start_min));
	 m_end_hour= gtk_spin_button_get_value(GTK_SPIN_BUTTON(spin_button_end_hour));
	 m_end_min= gtk_spin_button_get_value(GTK_SPIN_BUTTON(spin_button_end_min));
	 
	if (m_is_allday) 
	{
		m_start_hour=0; //sorting to top
		m_start_min=0;
		m_end_hour=0;
		m_end_min=0;
	 }
	
	//TODO multiday
	m_end_day=m_start_day;
	m_end_month=m_start_month;
	m_end_year=m_start_year;
		
	m_has_reminder = gtk_check_button_get_active(GTK_CHECK_BUTTON(check_button_hasreminder));
	m_reminder_hour= gtk_spin_button_get_value(GTK_SPIN_BUTTON(spin_button_reminder_hour));
	m_reminder_min= gtk_spin_button_get_value(GTK_SPIN_BUTTON(spin_button_reminder_min));
		
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
	
	g_object_set(selected_evt, "hasreminder", m_has_reminder, NULL);
	g_object_set(selected_evt, "reminderhour", m_reminder_hour, NULL);
	g_object_set(selected_evt, "remindermin", m_reminder_min, NULL);
	
	db_update_event(selected_evt);
	
	m_index = -1;
	m_id_selection = -1;
			
	set_titles_on_calendar(CUSTOM_CALENDAR(calendar));
	
	custom_calendar_update(CUSTOM_CALENDAR(calendar));
	
	GtkWidget *dialog_day_selected = g_object_get_data(G_OBJECT(window), "window-dialog-day-selected-key");
	
	gtk_window_destroy(GTK_WINDOW(dialog));
	gtk_window_destroy(GTK_WINDOW(dialog_day_selected));
	
}

//======================================================================
static void callbk_edit_event(GSimpleAction *action, GVariant *parameter,  gpointer user_data)
{
     
     //if(m_index<0) return;
     
     if (m_id_selection == -1) return;
	
	GtkWindow *window = user_data;
	GtkWidget *dialog;
	
	dialog = gtk_window_new();
	gtk_window_set_title(GTK_WINDOW(dialog), "Update Event");
	
	GtkWidget *button_update;	
	GtkWidget *grid;
	
	GtkWidget *label_summary;
	GtkWidget *dropdown_summary;
		
	GtkWidget *label_description;
	GtkWidget *entry_description;
	
	GtkWidget *label_location;
	GtkWidget *entry_location;
	
	GtkEntryBuffer *buffer_summary;
	GtkEntryBuffer *buffer_location;
	GtkEntryBuffer *buffer_description; 
	
	GtkWidget *label_spacer1;
	GtkWidget *label_spacer2;
	GtkWidget *label_spacer3;
	GtkWidget *label_spacer4;
	GtkWidget *label_spacer5;
	GtkWidget *label_spacer6;
	
	GtkWidget *dropdown_month_start;
	
	//start date
	GtkWidget *label_start_date;
	GtkWidget *spin_button_start_day;			
	GtkWidget *spin_button_start_year;
	
	// Check buttons
	GtkWidget *check_button_allday;	
	GtkWidget *check_button_isyearly;
	GtkWidget *check_button_priority;
	
	//Adjustments
	// value,lower,upper,step_increment,page_increment,page_size
	GtkAdjustment *adjustment_day_start = gtk_adjustment_new(1.00, 0.0, 31.00, 1.0, 1.0, 0.0);	
	GtkAdjustment *adjustment_year_start = gtk_adjustment_new(2024.00, 0.0, 5024.00, 1.0, 1.0, 0.0);
	
	//start time
	GtkWidget *label_start_time;
	GtkWidget *spin_button_start_hour;	
	GtkWidget *spin_button_start_min;
	//end time
	GtkWidget *label_end_time;
	GtkWidget *spin_button_end_hour;	
	GtkWidget *spin_button_end_min;	
	
	//reminder time
	GtkWidget *check_button_hasreminder;
	GtkWidget *label_reminder_time;	
	GtkWidget *spin_button_reminder_hour;	
	GtkWidget *spin_button_reminder_min;	
	
	GtkAdjustment *adjustment_start_hour = gtk_adjustment_new(1.00, 0.0, 23.00, 1.0, 1.0, 0.0);
	GtkAdjustment *adjustment_start_min= gtk_adjustment_new(1.00, 0.0, 59.00, 1.0, 1.0, 0.0);
	
	GtkAdjustment *adjustment_end_hour = gtk_adjustment_new(1.00, 0.0, 23.00, 1.0, 1.0, 0.0);
	GtkAdjustment *adjustment_end_min = gtk_adjustment_new(1.00, 0.0, 59.00, 1.0, 1.0, 0.0);
	
	GtkAdjustment *adjustment_reminder_hour = gtk_adjustment_new(1.00, 0.0, 23.00, 1.0, 1.0, 0.0);
	GtkAdjustment *adjustment_reminder_min= gtk_adjustment_new(1.00, 0.0, 59.00, 1.0, 1.0, 0.0);
	
	label_spacer1 = gtk_label_new("");
	label_spacer2 = gtk_label_new("");
	label_spacer3 = gtk_label_new("");
	label_spacer4 = gtk_label_new("");
	label_spacer5 = gtk_label_new("");
	label_spacer6 = gtk_label_new("");
	
	m_current_month=m_start_month;
	
	button_update = gtk_button_new_with_label ("Update Selected Event");
	g_signal_connect (GTK_BUTTON (button_update),"clicked", G_CALLBACK (callbk_update_event), G_OBJECT (window));
	
	grid = gtk_grid_new();	
	gtk_grid_set_column_homogeneous(GTK_GRID(grid), TRUE);
	
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
	gint reminder_hour = 0;
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
		
	g_object_get (selected_evt, "hasreminder", &has_reminder, NULL);
	g_object_get (selected_evt, "reminderhour", &reminder_hour, NULL);
	g_object_get (selected_evt, "remindermin", &reminder_min, NULL);
	
	m_summary = g_strdup(summary_str);
	m_location = g_strdup(location_str);
	m_description = g_strdup(description_str);
	m_start_year=start_year; 
	m_start_month=start_month;
	m_start_day=start_day;	
	m_start_hour = start_hour;
	m_start_min = start_min;
	m_end_hour = end_hour;
	m_end_min = end_min;
	m_is_yearly = is_yearly;
	m_is_allday = is_allday;	
	m_priority = is_priority;
	
	m_has_reminder=has_reminder;
	m_reminder_hour=reminder_hour;
	m_reminder_min=reminder_min;
	
	//Summary
	
	label_summary = gtk_label_new("Event Speech Word: ");
	dropdown_summary =gtk_drop_down_new_from_strings(events);    
	g_signal_connect(GTK_DROP_DOWN(dropdown_summary), "notify::selected", G_CALLBACK(callbk_dropdown_summary), NULL);
	
	guint position=0;
	
	position = get_dropdown_position_summary(m_summary);
	
	gtk_drop_down_set_selected(GTK_DROP_DOWN(dropdown_summary),position);
		
	//description
	label_description = gtk_label_new("Description: ");
	entry_description = gtk_entry_new();
	gtk_entry_set_has_frame(GTK_ENTRY(entry_description),TRUE); 
	gtk_entry_set_max_length(GTK_ENTRY(entry_description), 100);
	buffer_description = gtk_entry_buffer_new(m_description, -1); // show description
	gtk_entry_set_buffer(GTK_ENTRY(entry_description), buffer_description);

	//location
	label_location = gtk_label_new("Location: ");
	entry_location = gtk_entry_new();
	gtk_entry_set_has_frame(GTK_ENTRY(entry_location),TRUE); 
	gtk_entry_set_max_length(GTK_ENTRY(entry_location), 25);
	buffer_location = gtk_entry_buffer_new(m_location, -1); // show location
	gtk_entry_set_buffer(GTK_ENTRY(entry_location), buffer_location);

	//start date
	label_start_date =gtk_label_new("Start Date: ");
	
	spin_button_start_day = gtk_spin_button_new(adjustment_day_start, 1.0, 0);	
	g_signal_connect(GTK_SPIN_BUTTON(spin_button_start_day), "value_changed", G_CALLBACK(callbk_spin_day_start), NULL);	
	gtk_spin_button_set_value(GTK_SPIN_BUTTON(spin_button_start_day), m_start_day);
	
	dropdown_month_start =gtk_drop_down_new_from_strings(month_strs);    
    g_signal_connect(GTK_DROP_DOWN(dropdown_month_start), "notify::selected", G_CALLBACK(callbk_dropdown_month_start), NULL);
    gtk_drop_down_set_selected(GTK_DROP_DOWN(dropdown_month_start),m_start_month-1);
	
	spin_button_start_year = gtk_spin_button_new(adjustment_year_start, 1.0, 0);
	g_signal_connect(GTK_SPIN_BUTTON(spin_button_start_year), "value_changed", G_CALLBACK(callbk_spin_year_start), NULL);		
	gtk_spin_button_set_value(GTK_SPIN_BUTTON(spin_button_start_year), m_start_year);
			
	//Times
	//start time
	label_start_time =gtk_label_new("Start Time: ");	
	
	spin_button_start_hour = gtk_spin_button_new(adjustment_start_hour, 1.0, 0);
	gtk_spin_button_set_value(GTK_SPIN_BUTTON(spin_button_start_hour), m_start_hour);
	g_signal_connect(GTK_SPIN_BUTTON(spin_button_start_hour), "value_changed", G_CALLBACK(callbk_spin_hour_start), NULL);	
		
	spin_button_start_min = gtk_spin_button_new(adjustment_start_min, 1.0, 0);
	gtk_spin_button_set_value(GTK_SPIN_BUTTON(spin_button_start_min), m_start_min);	
	g_signal_connect(GTK_SPIN_BUTTON(spin_button_start_min), "value_changed", G_CALLBACK(callbk_spin_min_start), NULL);
		
	//end time
	label_end_time =gtk_label_new("End Time: ");	
	
	spin_button_end_hour = gtk_spin_button_new(adjustment_end_hour, 1.0, 0);
	gtk_spin_button_set_value(GTK_SPIN_BUTTON(spin_button_end_hour), m_end_hour); //end hour
	g_signal_connect(GTK_SPIN_BUTTON(spin_button_end_hour), "value_changed", G_CALLBACK(callbk_spin_hour_end), NULL);
	
	spin_button_end_min = gtk_spin_button_new(adjustment_end_min, 1.0, 0);
	gtk_spin_button_set_value(GTK_SPIN_BUTTON(spin_button_end_min), m_end_min); //end min
	g_signal_connect(GTK_SPIN_BUTTON(spin_button_end_min), "value_changed", G_CALLBACK(callbk_spin_min_end), NULL);
	
	//reminder time
	label_reminder_time =gtk_label_new("Reminder Time:");
	
	
	spin_button_reminder_hour = gtk_spin_button_new(adjustment_reminder_hour, 1.0, 0);	
	g_signal_connect(GTK_SPIN_BUTTON(spin_button_reminder_hour), "value_changed", G_CALLBACK(callbk_spin_hour_reminder), NULL);	
	gtk_spin_button_set_value(GTK_SPIN_BUTTON(spin_button_reminder_hour), m_reminder_hour);
	
	spin_button_reminder_min = gtk_spin_button_new(adjustment_reminder_min, 1.0, 0);		
	g_signal_connect(GTK_SPIN_BUTTON(spin_button_reminder_min), "value_changed", G_CALLBACK(callbk_spin_min_reminder), NULL);	
	gtk_spin_button_set_value(GTK_SPIN_BUTTON(spin_button_reminder_min), m_reminder_min);	
		
	
	check_button_hasreminder=gtk_check_button_new_with_label("Has Reminder");
	gtk_check_button_set_active(GTK_CHECK_BUTTON(check_button_hasreminder), m_has_reminder);
	
	g_signal_connect_swapped(GTK_CHECK_BUTTON(check_button_hasreminder), "toggled",
							 G_CALLBACK(callbk_check_button_hasreminder_toggled), check_button_hasreminder);
			
	
	g_object_set_data(G_OBJECT(check_button_hasreminder), "cb_hasreminder_spin_reminder_hour_key", spin_button_reminder_hour);
	g_object_set_data(G_OBJECT(check_button_hasreminder), "cb_hasreminder_spin_reminder_min_key", spin_button_reminder_min);						 
	
	
	
	check_button_allday = gtk_check_button_new_with_label("Is All Day");
	gtk_check_button_set_active(GTK_CHECK_BUTTON(check_button_allday), m_is_allday);
	g_signal_connect_swapped(GTK_CHECK_BUTTON(check_button_allday), "toggled",
							 G_CALLBACK(callbk_check_button_allday_toggled), check_button_allday);
	
	
	if (m_is_allday)
	{
		gtk_widget_set_sensitive(spin_button_start_hour, FALSE);
		gtk_widget_set_sensitive(spin_button_start_min, FALSE);
		gtk_widget_set_sensitive(spin_button_end_hour, FALSE);
		gtk_widget_set_sensitive(spin_button_end_min, FALSE);
	}
	else
	{
		gtk_widget_set_sensitive(spin_button_start_hour, TRUE);
		gtk_widget_set_sensitive(spin_button_start_min, TRUE);
		gtk_widget_set_sensitive(spin_button_end_hour, TRUE);
		gtk_widget_set_sensitive(spin_button_end_min, TRUE);
	}
	
	
	g_object_set_data(G_OBJECT(check_button_allday), "cb_allday_spin_start_hour_key", spin_button_start_hour);
	g_object_set_data(G_OBJECT(check_button_allday), "cb_allday_spin_start_min_key", spin_button_start_min);
	g_object_set_data(G_OBJECT(check_button_allday), "cb_allday_spin_end_hour_key", spin_button_end_hour);
	g_object_set_data(G_OBJECT(check_button_allday), "cb_allday_spin_end_min_key", spin_button_end_min);
		
	
	check_button_isyearly = gtk_check_button_new_with_label("Is Yearly");
	check_button_priority = gtk_check_button_new_with_label("Is High Priority");
	
	gtk_check_button_set_active(GTK_CHECK_BUTTON(check_button_isyearly), m_is_yearly);	
	gtk_check_button_set_active(GTK_CHECK_BUTTON(check_button_priority), m_priority);
	
	g_object_set_data(G_OBJECT(button_update), "dialog-key", dialog);	
	//g_object_set_data(G_OBJECT(button_update), "dropdown-summary-key", dropdown_summary);
	g_object_set_data(G_OBJECT(button_update), "entry-location-key", entry_location);	
	g_object_set_data(G_OBJECT(button_update), "entry-description-key", entry_description);
	
	g_object_set_data(G_OBJECT(button_update), "spin-start-hour-key", spin_button_start_hour);
	g_object_set_data(G_OBJECT(button_update), "spin-start-min-key", spin_button_start_min);
	g_object_set_data(G_OBJECT(button_update), "spin-end-hour-key", spin_button_end_hour);
	g_object_set_data(G_OBJECT(button_update), "spin-end-min-key", spin_button_end_min);
	
	//reminder
	g_object_set_data(G_OBJECT(button_update), "check-button-hasreminder-key", check_button_hasreminder);
	g_object_set_data(G_OBJECT(button_update), "spin-reminder-hour-key", spin_button_reminder_hour);
	g_object_set_data(G_OBJECT(button_update), "spin-reminder-min-key", spin_button_reminder_min);
		
	
	
	g_object_set_data(G_OBJECT(button_update), "spin-day-start-key", spin_button_start_day);	
	g_object_set_data(G_OBJECT(button_update), "spin-year-start-key", spin_button_start_year);	
	
	g_object_set_data(G_OBJECT(button_update), "check-button-allday-key", check_button_allday);	
	g_object_set_data(G_OBJECT(button_update), "check-button-isyearly-key", check_button_isyearly);
	g_object_set_data(G_OBJECT(button_update), "check-button-priority-key", check_button_priority);
	
	//grid layout
	gtk_grid_attach(GTK_GRID(grid), label_summary, 1, 1, 1, 1);
	gtk_grid_attach(GTK_GRID(grid), dropdown_summary, 2, 1, 1, 1);
	
	gtk_grid_attach(GTK_GRID(grid), label_description, 1, 2, 1, 1);
	gtk_grid_attach(GTK_GRID(grid), entry_description, 2, 2, 3, 1);
		
	gtk_grid_attach(GTK_GRID(grid), label_location, 1, 3, 1, 1);
	gtk_grid_attach(GTK_GRID(grid), entry_location, 2, 3, 3, 1);
	
	gtk_grid_attach(GTK_GRID(grid), label_spacer1,       1, 4, 3, 1);
	//start date
	gtk_grid_attach(GTK_GRID(grid), label_start_date,       1, 5, 1, 1);
	gtk_grid_attach(GTK_GRID(grid), spin_button_start_day,  2, 5, 1, 1);	
	gtk_grid_attach(GTK_GRID(grid), dropdown_month_start,    3, 5, 1, 1);
	gtk_grid_attach(GTK_GRID(grid), spin_button_start_year,  4, 5, 1, 1);	
	
	gtk_grid_attach(GTK_GRID(grid), label_spacer2,       1, 6, 3, 1);
	
	//start time
	gtk_grid_attach(GTK_GRID(grid), label_start_time,       1, 7, 1, 1);
	gtk_grid_attach(GTK_GRID(grid), spin_button_start_hour,  2, 7, 1, 1);
	gtk_grid_attach(GTK_GRID(grid), spin_button_start_min,   3, 7, 1, 1);
	//end time
	gtk_grid_attach(GTK_GRID(grid), label_end_time,        1, 8, 1, 1);
	gtk_grid_attach(GTK_GRID(grid), spin_button_end_hour,  2, 8, 1, 1);
	gtk_grid_attach(GTK_GRID(grid), spin_button_end_min,   3, 8, 1, 1);
	
	gtk_grid_attach(GTK_GRID(grid), label_spacer3,       1, 9, 3, 1);
		
	//reminder time
	gtk_grid_attach(GTK_GRID(grid), check_button_hasreminder,       1, 10, 1, 1);
	gtk_grid_attach(GTK_GRID(grid), label_reminder_time,            2, 10, 1, 1);
	gtk_grid_attach(GTK_GRID(grid), spin_button_reminder_hour,      3, 10, 1, 1);
	gtk_grid_attach(GTK_GRID(grid), spin_button_reminder_min,        4, 10, 1, 1);
	
	gtk_grid_attach(GTK_GRID(grid), label_spacer4,       1, 11, 3, 1); 
	

	gtk_grid_attach(GTK_GRID(grid), check_button_allday,        1, 12, 1, 1);
    gtk_grid_attach(GTK_GRID(grid), check_button_isyearly,      2, 12, 1, 1);  
    gtk_grid_attach(GTK_GRID(grid), check_button_priority,      3, 12, 1, 1);
       
    gtk_grid_attach(GTK_GRID(grid), label_spacer5,       1, 13, 3, 1);
	
	gtk_grid_attach(GTK_GRID(grid), button_update,  1, 14, 4, 1);

    gtk_window_set_child (GTK_WINDOW (dialog), grid);	
	gtk_window_present(GTK_WINDOW(dialog));   	
  
}
//======================================================================

static void callbk_delete_event(GtkButton *button, gpointer  user_data)
{  
    
   if(m_index <0) return;
  
   GtkWindow *window =user_data;
   GtkWidget *calendar =g_object_get_data(G_OBJECT(window), "window-calendar-key");
  
   GtkWidget *dialog = g_object_get_data(G_OBJECT(button), "dialog-delete-key");

	int selected_evt_id=0;
	g_object_get (selected_evt, "eventid", &selected_evt_id, NULL);	
	
	db_delete_row(selected_evt_id);
	m_index=-1; //listview index
	m_id_selection=-1;
	
	//update calendar
	set_titles_on_calendar(CUSTOM_CALENDAR(calendar));	
	custom_calendar_update(CUSTOM_CALENDAR(calendar));	
    gtk_window_destroy(GTK_WINDOW(dialog)); 
 
}


//======================================================================
// Day selected
//======================================================================

static void callbk_calendar_day_selected(CustomCalendar *calendar, gpointer user_data)
{
	m_start_day = custom_calendar_get_day(CUSTOM_CALENDAR(calendar));
	m_start_month = custom_calendar_get_month(CUSTOM_CALENDAR(calendar));
	m_start_year = custom_calendar_get_year(CUSTOM_CALENDAR(calendar));	
	
	GtkWidget *window =user_data;
	GtkWidget *dialog_day_selected;		
	
	GtkWidget *listbox, *box, *icon, *label;
	
	dialog_day_selected =gtk_window_new(); 
	gtk_window_set_title (GTK_WINDOW (dialog_day_selected), "Day Events");
	gtk_window_set_default_size(GTK_WINDOW(dialog_day_selected),600,300);
	
	//window stacking order determined by window manager	
	//gtk_window_set_modal(GTK_WINDOW (dialog_day_selected),TRUE); //causes the update event dialog to freeze	
	//centre dialog window?
	gtk_window_set_transient_for(GTK_WINDOW (dialog_day_selected),GTK_WINDOW(window));	
	
	GtkWidget *header;   
	GtkWidget *button_edit_event;
	GtkWidget *button_delete_event;
		
	header = gtk_header_bar_new ();  
	gtk_window_set_titlebar (GTK_WINDOW (dialog_day_selected), header);
	
	g_object_set_data(G_OBJECT(window), "window-dialog-day-selected-key",dialog_day_selected);	  
	
	button_edit_event = gtk_button_new_with_label ("Edit Event");
	g_signal_connect (button_edit_event, "clicked", G_CALLBACK (callbk_edit_event), window); 
	
	button_delete_event = gtk_button_new_with_label ("Delete Event"); 
	g_signal_connect (button_delete_event, "clicked", G_CALLBACK (callbk_delete_event), window);
    
	gtk_header_bar_pack_start(GTK_HEADER_BAR (header), button_edit_event);
	gtk_header_bar_pack_start(GTK_HEADER_BAR (header), button_delete_event);
		
	
	listbox = gtk_list_box_new ();
	gtk_list_box_set_selection_mode(GTK_LIST_BOX(listbox), GTK_SELECTION_SINGLE);
	g_signal_connect (listbox, "row-activated", G_CALLBACK (callbk_row_activated),NULL);
	
	GArray *evt_arry_day; //day events	
	evt_arry_day = g_array_new(FALSE, FALSE, sizeof(CALENDAR_TYPE_EVENT)); // setup arraylist
	db_get_all_events_year_month_day(evt_arry_day, m_start_year,m_start_month, m_start_day);
	
	//display
	for (int i = 0; i < evt_arry_day->len; i++)
	{
	CalendarEvent *evt_day = g_array_index(evt_arry_day , CalendarEvent *, i);
	
	int start_day=0;
	int start_month=0;
	int start_year=0;
	char* summary_str="";	
	char *location_str="";
	char *description_str="";	
	int start_hour=0;
	int start_min=0;	
	int end_hour=0;
	int end_min=0;
	int is_yearly=0;
	int is_allday=0;
	int is_priority=0;
	
	g_object_get (evt_day, "startday", &start_day, NULL);
	g_object_get (evt_day, "startmonth", &start_month, NULL);
	g_object_get (evt_day, "startyear", &start_year, NULL);	
	g_object_get(evt_day, "summary", &summary_str, NULL);	
	g_object_get(evt_day, "location", &location_str, NULL);
	g_object_get(evt_day, "description", &description_str, NULL);	
	g_object_get(evt_day, "starthour", &start_hour, NULL);
	g_object_get(evt_day, "startmin", &start_min, NULL);	
	g_object_get(evt_day, "endhour", &end_hour, NULL);
	g_object_get(evt_day, "endmin", &end_min, NULL);
	g_object_get(evt_day, "isyearly", &is_yearly, NULL);
	g_object_get(evt_day, "isallday", &is_allday, NULL);
	g_object_get(evt_day, "ispriority", &is_priority, NULL);
	
	char *display_str="";
	char *time_str = "";
	char *end_time_str = "";
	char *starthour_str = "";
	char *startmin_str = "";
	char *endhour_str = "";
	char *endmin_str = "";
	char *ampm_str = " ";
	
	if(!is_allday)
	{		
	 time_str =get_time_str(start_hour,start_min);
	
	//------------------------------------------------------------------
	if (m_show_end_time)
	{	
	end_time_str =get_time_str(end_hour,end_min);
	time_str = g_strconcat(time_str, "to ", end_time_str, NULL);
	}	
	display_str = g_strconcat(display_str, time_str, summary_str, "\n", NULL);	
		
	}//if not allday
	
	else
	{
		display_str=g_strconcat(display_str,summary_str, "\n",NULL);
	}
			
	if (!strlen(description_str) == 0){
	display_str = g_strconcat(display_str,description_str, "\n", NULL);
	}
	
	if (!strlen(location_str) == 0){
	display_str = g_strconcat(display_str,location_str, "\n", NULL);
	}
	if (is_priority)
	{
	display_str = g_strconcat(display_str, "High Priority.","\n", NULL);
	}
		
	box = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 6);
	gtk_list_box_append (GTK_LIST_BOX (listbox), box);	
	label = gtk_label_new (display_str);
	gtk_box_append (GTK_BOX (box), label);	
    
    }//for day events
			
	g_array_free(evt_arry_day, FALSE); //clear the array 
	
	if (m_speaking == FALSE) speak_events();  //i.e. not already speaking
    
    g_object_set_data(G_OBJECT(button_delete_event), "dialog-delete-key", dialog_day_selected);
    
	gtk_window_set_child (GTK_WINDOW (dialog_day_selected),listbox);
	gtk_window_present (GTK_WINDOW (dialog_day_selected));	   
    				
}

//======================================================================
// new approach to get_upcoming_array
//======================================================================

GArray*  get_upcoming_array(int upcoming_days) 
{
	
	//if(upcoming_days >14) upcoming_days=14;	
	GArray *evt_arry_upcoming;
	evt_arry_upcoming = g_array_new(FALSE, FALSE, sizeof(CALENDAR_TYPE_EVENT)); 
	
	GDate *today_date;
	today_date = g_date_new();
	g_date_set_time_t(today_date, time(NULL));
	int today = g_date_get_day(today_date);
	int month = g_date_get_month(today_date);
	int year = g_date_get_year(today_date);
	g_date_free(today_date); // freeit quick
		
	GDate* date =g_date_new_dmy(today, month, year);
	g_date_add_days(date, 1); //start at next day
	
	int loop_days=upcoming_days;
		
	//use while loop
	while(loop_days >=0)
	{	
	
	int day =g_date_get_day (date);
	int month=g_date_get_month (date);
	int year =g_date_get_year (date);
		
	GArray *evt_arry_day;	
	evt_arry_day = g_array_new(FALSE, FALSE, sizeof(CALENDAR_TYPE_EVENT)); // setup arraylist
	
	db_get_all_events_year_month_day(evt_arry_day, year,month,day);
	
	for (int i = 0; i < evt_arry_day->len; i++)
	{
	CalendarEvent *evt = g_array_index(evt_arry_day, CalendarEvent *, i);
	
	char* summary_str;
	g_object_get(evt, "summary", &summary_str, NULL);	
	g_array_append_val(evt_arry_upcoming, evt);	  //!!
		
	}//for
    	
	g_array_free(evt_arry_day, FALSE); //clear the day array 
	g_date_add_days(date, 1);				
	loop_days=loop_days-1;
	
	} //while
	// g_array_free(evt_arry_upcoming, TRUE); //calling funtion to free
	
	if (evt_arry_upcoming!=NULL) return evt_arry_upcoming;
	else return NULL;
 
} 

//======================================================================
// old approach to get_upcoming_array
//======================================================================

//GArray*  get_upcoming_array(int upcoming_days) 
//{
	//if(upcoming_days > 7) upcoming_days=7; //clamp
	
	//int day1=1; //start day
	//int day2=1; //end day
		
	//GDate *today_date;
	//today_date = g_date_new();
	//g_date_set_time_t(today_date, time(NULL));
	//int today = g_date_get_day(today_date);
	//int month = g_date_get_month(today_date);
	//int year = g_date_get_year(today_date);
	//g_date_free(today_date); // freeit quick
	
	//guint8 month_days =g_date_get_days_in_month(month,year);	
		
	//if(((today+1)+upcoming_days)<=month_days)
	//{
		////Normal case
		//GArray *evt_arry_upcoming;
	    //evt_arry_upcoming = g_array_new(FALSE, FALSE, sizeof(CALENDAR_TYPE_EVENT)); 	
		
		//day1=today+1;
		//day2=today+upcoming_days;
		//db_get_upcoming_events(evt_arry_upcoming,year,month,day1,day2);	
	    //return evt_arry_upcoming;		
	//}
	
	//else {
		////special edge cases 
		
		//if (today == month_days){
		////Special case: today is last day of month 
		////so today+1 will be first day of next month
		
		//GArray *evt_arry_upcoming;
		//evt_arry_upcoming = g_array_new(FALSE, FALSE, sizeof(CALENDAR_TYPE_EVENT)); 
			
		//day1=1;
		//day2=upcoming_days;
		//month=month+1; //next month
		
		//if(month>12) { //december then becomes january
			//month =1; 	
			//year=year+1;
		//}
		
		//db_get_upcoming_events(evt_arry_upcoming,year,month,day1,day2);	
		//return evt_arry_upcoming;	
		//}
		//else
		//{	
		////complex two month case
		//int day1=today+1;
		
		//int day_diff=(today+upcoming_days)-month_days;		
		
		//GArray *total_arry;
	    //total_arry= g_array_new(FALSE, FALSE, sizeof(CALENDAR_TYPE_EVENT)); 
	    
	    //GArray *month1_arry;
	    //month1_arry= g_array_new(FALSE, FALSE, sizeof(CALENDAR_TYPE_EVENT));
	    //db_get_upcoming_events(month1_arry,year,month,day1,month_days); 
	    
	    ////return month1_arry;
	    ////Append month1_arry to total_arry
	    //for (int i = 0; i < month1_arry->len; i++)
	    //{
		//CalendarEvent *evt = g_array_index(month1_arry, CalendarEvent *, i);
		//g_array_append_val(total_arry, evt);		
		//}
	   
	    //int next_month=1;
	    ////find next month and year
	    //if(month+1>12) {
	    //next_month=1;
	    //year=year+1;
	    //} 
	    //else {
			//next_month=month+1;
		//}
	     
	    //GArray *month2_arry;
	    //month2_arry= g_array_new(FALSE, FALSE, sizeof(CALENDAR_TYPE_EVENT)); 
	    //db_get_upcoming_events(month2_arry,year,next_month,1,day_diff); 
	    
	    ////Append  month2_arry to total arry	    
	    //for (int j = 0; j < month2_arry->len; j++)
	    //{
		//CalendarEvent *evt = g_array_index(month2_arry, CalendarEvent *, j);
		//g_array_append_val(total_arry, evt);		
		//}	
		//return total_arry;	
		
		//} //else 2 month edge case
		
	//}//else special cases
	
//return NULL; //should never get here but if it does return NULL
//}

//======================================================================

//----------------------------------------------------------------
// Callback home (go to current date)
//-----------------------------------------------------------------
static void callbk_calendar_home(GSimpleAction * action, GVariant *parameter, gpointer user_data)
{
	GtkWindow *window =user_data;	
	GtkWidget *calendar =g_object_get_data(G_OBJECT(window), "window-calendar-key");
	
	custom_calendar_goto_today(CUSTOM_CALENDAR(calendar));
	
	m_today_day = custom_calendar_get_day(CUSTOM_CALENDAR(calendar));
	m_today_month = custom_calendar_get_month(CUSTOM_CALENDAR(calendar));
	m_today_year = custom_calendar_get_year(CUSTOM_CALENDAR(calendar));		
	
	m_start_day=m_today_day;
	m_start_month=m_today_month;
	m_start_year=m_today_year;
	
	m_id_selection = -1;	
	m_index=-1;
	//mark up calendar	
	
	set_titles_on_calendar(CUSTOM_CALENDAR(calendar));
	
	custom_calendar_update(CUSTOM_CALENDAR(calendar));		
}

//======================================================================

static void callbk_confirm_delete_all(GtkButton *button, gpointer  user_data)
{	
	GtkWindow *window =user_data;	
	GtkWidget *calendar =g_object_get_data(G_OBJECT(window), "window-calendar-key");
	
	GtkWidget *dialog = g_object_get_data(G_OBJECT(button), "dialog-key");
	
	db_delete_all();
			
	 set_titles_on_calendar(CUSTOM_CALENDAR(calendar));  		
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
//Export ical file
//======================================================================

static void callbk_export(GSimpleAction *action, GVariant *parameter,  gpointer user_data)
{
	
	GtkWidget *window = user_data;
	
	 GtkFileDialog *dialog; //export file dialog
     dialog = gtk_file_dialog_new(); 
     gtk_file_dialog_set_title (dialog,"Export ical calendar file");  
     gtk_file_dialog_set_initial_name (dialog, "speakingcalendar.ical");
     
    GtkFileFilter *filefilter0 = gtk_file_filter_new();
	gtk_file_filter_set_name(filefilter0,"All Files");
    gtk_file_filter_add_pattern(filefilter0, "*");
    
    GtkFileFilter* filefilter1 = gtk_file_filter_new();
	gtk_file_filter_add_suffix(filefilter1,"ical");
	gtk_file_filter_set_name(filefilter1,"ical");
	
	GtkFileFilter *filefilter2 = gtk_file_filter_new();
	gtk_file_filter_add_suffix(filefilter2,"ics");
	gtk_file_filter_set_name(filefilter2,"ics");
	
	GtkFileFilter *filefilter3 = gtk_file_filter_new();
	gtk_file_filter_add_suffix(filefilter3,"ifb");
	gtk_file_filter_set_name(filefilter3,"ifb");
	
	GtkFileFilter *filefilter4 = gtk_file_filter_new();
	gtk_file_filter_add_suffix(filefilter4,"icalendar");
	gtk_file_filter_set_name(filefilter4,"icalendar");
	
	GtkFileFilter* filefilter5 = gtk_file_filter_new();
	gtk_file_filter_add_suffix(filefilter5,"txt");
	gtk_file_filter_set_name(filefilter5,"Text");
	
	GListStore* liststore = g_list_store_new (GTK_TYPE_FILE_FILTER);
	g_list_store_append(liststore, filefilter0);
	g_list_store_append(liststore, filefilter1);
	g_list_store_append(liststore, filefilter2);
	g_list_store_append(liststore, filefilter3);
	g_list_store_append(liststore, filefilter4);
	g_list_store_append(liststore, filefilter5);
		
	
	gtk_file_dialog_set_filters(dialog,G_LIST_MODEL(liststore));    
	gtk_file_dialog_save(dialog, NULL, NULL, file_save_response, NULL); //no longer signal based
	g_object_set_data(G_OBJECT(dialog), "dialog-window-key",window);	
	g_object_unref (dialog);
}
//======================================================================
// GAsync callback
//======================================================================
static void file_save_response (GObject *source, GAsyncResult *result, void *user_data)
{
  
  GtkFileDialog *dialog = GTK_FILE_DIALOG (source);  
  
  GtkWidget *window = g_object_get_data(G_OBJECT(dialog), "dialog-window-key");
 
  GFile *file;
  file = gtk_file_dialog_save_finish (dialog, result, NULL);
	  if (file)
	  {	 
	  char *file_name = g_file_get_path(file);	  
	  export_file(file_name);
	  }
}
//=====================================================================
void export_file(char *file_name) 
{
	GFile *file;
	GFileOutputStream *file_stream;
	GDataOutputStream *data_stream;
	GError *err = NULL;
	
	file = g_file_new_for_path(file_name);

	file_stream = g_file_replace(file, 0,TRUE, G_FILE_CREATE_NONE, NULL, &err);
	    
	if (file_stream == NULL) {
                
				//gint errno = err->code;
				g_warning ("Error message = %s", err->message);
                g_error_free (err);
                g_print("Unable to open file: %s\n",file_name);
                g_object_unref (file);
                return;
        }

	data_stream = g_data_output_stream_new(G_OUTPUT_STREAM(file_stream));
	
	GArray *evt_arry =g_array_new(FALSE, FALSE, sizeof(CALENDAR_TYPE_EVENT));
	db_get_all_events(evt_arry);
	
	
	char *line = "";
	line = g_strconcat(line,"BEGIN:VCALENDAR\n",NULL);	
	g_data_output_stream_put_string(data_stream, line, NULL, NULL);

	for (int i = 0; i < evt_arry->len; i++)
	{		
		line="";
		gint evt_id = 0;
		gchar *summary_str = "";
		gchar *location_str = "";
		gchar *description_str = "";

		gint start_year = 0;
		gint start_month = 0;
		gint start_day = 0;
		gint start_hour = 0;
		gint start_min = 0;
		gint start_seconds = 0;

		gint end_year = 0;
		gint end_month = 0;
		gint end_day = 0;
		gint end_hour = 0;
		gint end_min = 0;
		gint end_seconds = 0;

		gint is_yearly = 0;
		gint is_allday = 0;
		gint is_multiday = 0;
		gint is_priority = 0;
		
		gint has_reminder = 0;
		gint reminder_hour = 0;
		gint reminder_min = 0;
		char* recurrence_str="RRULE:";

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
		
		g_object_get(evt, "hasreminder", &has_reminder, NULL);
		g_object_get(evt, "reminderhour", &reminder_hour, NULL);
		g_object_get(evt, "remindermin", &reminder_min, NULL);
	
		char* start_day_str = g_strdup_printf("%02d",start_day);
		char* start_month_str = g_strdup_printf("%02d",start_month);
		char* start_year_str = g_strdup_printf("%d",start_year);
		char* start_hour_str = g_strdup_printf("%02d",start_hour);
		char* start_min_str = g_strdup_printf("%02d",start_min);
		char* start_sec_str = g_strdup_printf("%02d",start_seconds);
		
		char* end_day_str = g_strdup_printf("%02d",end_day);
		char* end_month_str = g_strdup_printf("%02d",end_month);
		char* end_year_str = g_strdup_printf("%d",end_year);
		char* end_hour_str = g_strdup_printf("%02d",end_hour);
		char* end_min_str = g_strdup_printf("%02d",end_min);
		char* end_sec_str = g_strdup_printf("%02d",end_seconds);
		
		char* priority_str="";
		if (is_priority ==1) priority_str="PRIORITY:1";
		else priority_str="PRIORITY:0";
		
		//recurrence rule (more todo)
		
		if(is_yearly)
		{					
			recurrence_str = g_strconcat(recurrence_str,
		    "FREQ=YEARLY;",
		    "INTERVAL=1;",
		    "BYMONTH=",
		    start_month_str,
		    ";",
		    "BYMONTHDAY=",
		    start_day_str,	   
		     NULL);			  
		}
		
		char *dts ="DTSTART:";		
		dts = g_strconcat(dts,
		start_year_str,
		start_month_str,
		start_day_str,
		"T",
		start_hour_str,
		start_min_str,
		start_sec_str,
		"\n",
		NULL);
		
		char *dte ="DTEND:";		
		dte = g_strconcat(dte,
		end_year_str,
		end_month_str,
		end_day_str,
		"T",
		end_hour_str,
		end_min_str,
		end_sec_str,
		"\n",
		NULL);
		
		
		line = g_strconcat(line,"BEGIN:VEVENT\n",NULL);
		
		line = g_strconcat(line,dts,NULL);
		line = g_strconcat(line,dte,NULL);
		line = g_strconcat(line,"LOCATION:",location_str, "\n", NULL);
		line = g_strconcat(line,"SUMMARY:",summary_str,"\n", NULL);
		line = g_strconcat(line,"DESCRIPTION:",description_str,"\n", NULL);
		
		line = g_strconcat(line,priority_str,"\n", NULL);
		
		if(is_yearly)
		{
			
		line = g_strconcat(line,recurrence_str,"\n", NULL);
		is_yearly=0; //prevent next line repeats
		}
		
		line = g_strconcat(line,"END:VEVENT\n",NULL);		
		g_data_output_stream_put_string(data_stream, line, NULL, NULL);		
		
	}//for evt_arry
	
	line ="";
	line = g_strconcat(line,"END:VCALENDAR\n",NULL);
	g_data_output_stream_put_string(data_stream, line, NULL, NULL);

	g_object_unref(data_stream);
	g_object_unref(file_stream);
	g_object_unref(file);
		
}
//======================================================================


//======================================================================
// Import ical file
//======================================================================
gboolean import_ical_file(char* file_name, gpointer user_data) 
{
	
		
	GtkWidget *window = user_data; //need window to get calendar
	GtkWidget *calendar = g_object_get_data(G_OBJECT(window), "window-calendar-key");
	
	//Two stage parser
	//Stage 1 checks for timezone
	//Stage2 extracts event details
	
	
	GFile *file;
	//GFileInputStream creates a stream of input that you can use to read data from a file
	GFileInputStream *file_stream = NULL;
	GDataInputStream *input_stream = NULL;	
	file = g_file_new_for_path(file_name);
	
	file_stream = g_file_read(file, NULL, NULL);
	if (!file_stream)
	{
		g_print("CRITICAL: error: unable to open backup file called example.ics\n");
		//return;
		return false;
	}
	else {
		//g_print("successfully opened: %s\n", file_name);
	}

	input_stream = g_data_input_stream_new(G_INPUT_STREAM(file_stream));
	
	char *key="";     
    char *value="";
    
	char *summary_str = "";
	char *event_number_str="";
	char *location_str = "";
	char *description_str = "";
	
	int start_day=0;
	int start_month=0;
	int start_year=0;
	
	int end_day=0;
	int end_month=0;
	int end_year=0;
	
	int start_hour = 0;
	int start_min = 0;
	
	int end_hour = 0;
	int end_min = 0;
	
	int is_allday = 0;		
	int is_priority = 0;
	int is_yearly=0;
	
	int has_reminder=0; //todo
	int reminder_hour=0;
	int reminder_min=0;
		
	
	gboolean is_timezone=0;
	char *timezone_str="";
	
	//Stage1: scan for timezone
	
	gchar *line1=""; //parser1
	
	while ((line1 = g_data_input_stream_read_line (input_stream, NULL, NULL, NULL))) {
	
		g_strstrip (line1);			
		
		gchar** tokens = g_strsplit (line1, ":", -1);
		key =tokens[0];
		value =tokens[1];
		
		if ((strcmp (key,"BEGIN") == 0) && (strcmp (value,"VTIMEZONE") == 0)) //check for timezone
		{
		is_timezone=1;
		}
		if (g_strcmp0 (key,"TZID") == 0)
		{				
		timezone_str=g_strdup(value);
		}
		
		g_free(line1);	
	}//while timezone check
			
	//Stage 2: extract event details
	if(is_timezone) //ical file has timezone
	{		
		//parse ical file which has a time zone
		char* dtstart_key ="DTSTART;";
		dtstart_key=g_strconcat(dtstart_key,"TZID=",timezone_str,NULL);
		char* dtend_key="DTEND;";
		dtend_key=g_strconcat(dtend_key,"TZID=",timezone_str,NULL);
		
		
		//reload file stream (stage 2 parse)
		file_stream = g_file_read(file, NULL, NULL);		
		input_stream = g_data_input_stream_new(G_INPUT_STREAM(file_stream));
		
		char* line2=""; //parser2
		while ((line2 = g_data_input_stream_read_line (input_stream, NULL, NULL, NULL))) {
				
			g_strstrip (line2);			
			gchar** tokens = g_strsplit (line2, ":", -1);
			key =tokens[0];
			value =tokens[1];			
			
			if (g_strcmp0 (key,dtstart_key) == 0)  //date time start
	        {	
				const char* start_date_str =g_strdup(value);				
				char* start_year_substring="";
				char* start_month_substring="";
				char* start_day_substring="";
				char* start_hour_str="";
				char* start_min_str="";
			
				start_year_substring = g_utf8_substring (start_date_str,0,4);
				start_month_substring = g_utf8_substring (start_date_str,4,6);
				start_day_substring = g_utf8_substring (start_date_str,6,8);
				
				start_hour_str = g_utf8_substring (start_date_str,9,11);
				start_min_str = g_utf8_substring (start_date_str,11,13);
				
				start_month_substring =ignore_first_zero(start_month_substring);
				start_day_substring =ignore_first_zero(start_day_substring);				
				start_hour_str =ignore_first_zero(start_hour_str);
				start_min_str =ignore_first_zero(start_min_str);
				
				if (strcmp (start_hour_str,"") == 0) start_hour_str ="0";
				if (strcmp (start_min_str,"") == 0) start_min_str ="0";
								
				//convert str to int using str-to-long-int
				start_year=g_ascii_strtoll(start_year_substring, NULL, 0);
				start_month=g_ascii_strtoll(start_month_substring, NULL, 0);
				start_day=g_ascii_strtoll(start_day_substring, NULL, 0);
				
				start_hour=g_ascii_strtoll(start_hour_str, NULL, 0);
				start_min=g_ascii_strtoll(start_min_str, NULL, 0);	
										
	        } //if dtstart
	        
	        if (g_strcmp0 (key,dtend_key) == 0)  //date time end
	        {	
				const char* end_date_str =g_strdup(value);				
				char* end_year_substring="";
				char* end_month_substring="";
				char* end_day_substring="";
				char* end_hour_str="";
				char* end_min_str="";
			
				end_year_substring = g_utf8_substring (end_date_str,0,4);
				end_month_substring = g_utf8_substring (end_date_str,4,6);
				end_day_substring = g_utf8_substring (end_date_str,6,8);
				
				end_hour_str = g_utf8_substring (end_date_str,9,11);
				end_min_str = g_utf8_substring (end_date_str,11,13);
				
				end_month_substring =ignore_first_zero(end_month_substring);
				end_day_substring =ignore_first_zero(end_day_substring);				
				end_hour_str =ignore_first_zero(end_hour_str);
				end_min_str =ignore_first_zero(end_min_str);
				
				if (strcmp (end_hour_str,"") == 0) end_hour_str ="0";
				if (strcmp (end_min_str,"") == 0) end_min_str ="0";
								
				//convert str to int using str-to-long-int
				end_year=g_ascii_strtoll(end_year_substring, NULL, 0);
				end_month=g_ascii_strtoll(end_month_substring, NULL, 0);
				end_day=g_ascii_strtoll(end_day_substring, NULL, 0);
				
				end_hour=g_ascii_strtoll(end_hour_str, NULL, 0);
				end_min=g_ascii_strtoll(end_min_str, NULL, 0);	
								
	        } //if dtend	
			
			if (g_strcmp0 (key,"SUMMARY") == 0)
			{				
				summary_str=g_strdup(value);
				
			}
			if (g_strcmp0 (key,"LOCATION") == 0)
	        {				
				location_str=g_strdup(value);
				
	        }
	        if (g_strcmp0 (key,"DESCRIPTION") == 0)
	        {				
				description_str=g_strdup(value);
				
	        }
	        
	        if (strcmp (key,"PRIORITY") == 0)
	       {
			   const char* priority_str =g_strdup(value);
			   int p_num=g_ascii_strtoll(priority_str, NULL, 0);
			   if (p_num >0) is_priority=1;
			   else is_priority=0;			  
		   }
		   
		   
		   if (strcmp (key,"RRULE") == 0)
		   {
			    const char* rule_str =g_strdup(value);			   			    
			   //YEARLY is the only RRULE recurring event currently supported
			   //More work needed here.			    
			   if (g_strrstr(rule_str,"FREQ=YEARLY") != NULL)
			   {
				   is_yearly=1;
			   }			   
		   }
		   
	        ////event ends and so insert into db	        
			if ((strcmp (key,"END") == 0) && (strcmp (value,"VEVENT") == 0)) //VEVENT ends
			{
							
				//is_allday=0;
				//if((start_hour==0) 
				//&& (start_min==0)
				//&& (end_hour==0)
				//&&(end_min==0))
				//{
					//is_allday=1;
				//}	
				
				is_allday=1;
				if((start_hour!=0) && (start_min!=0))				
				{
					is_allday=0;
				}				
				//VALARM to do
				
				CalendarEvent *evt = g_object_new(CALENDAR_TYPE_EVENT, 0);
				
				g_object_set(evt, "summary", g_strdup(summary_str), NULL);
				g_object_set(evt, "location", g_strdup(location_str), NULL);
				g_object_set(evt, "description", g_strdup(description_str), NULL);
				g_object_set(evt, "startyear", start_year, NULL);
				g_object_set(evt, "startmonth", start_month, NULL);
				g_object_set(evt, "startday", start_day, NULL);
				g_object_set(evt, "starthour", start_hour, NULL);
				g_object_set(evt, "startmin", start_min, NULL);
				g_object_set(evt, "endyear", end_year, NULL); // to do
				g_object_set(evt, "endmonth", end_month, NULL);
				g_object_set(evt, "endday", end_day, NULL);
				g_object_set(evt, "endhour", end_hour, NULL);
				g_object_set(evt, "endmin", end_min, NULL);
				g_object_set(evt, "isyearly", is_yearly, NULL);
				g_object_set(evt, "isallday", is_allday, NULL);			
				g_object_set(evt, "ispriority", is_priority, NULL);
				
				g_object_set(evt, "hasreminder", has_reminder, NULL); //todo
				g_object_set(evt, "reminderhour",reminder_hour, NULL);
				g_object_set(evt, "remindermin", reminder_min, NULL);	
				
				db_insert_event(evt); //insert event into database	
				
				//reset is_yearly for next event
				is_allday=0;
				is_yearly=0;
				is_priority=0;	
			}	//VEVENT END	
	        
			g_free(line2);
		} //while parse loop
		
		//g_free
		g_free(dtstart_key);
		g_free(dtend_key);
		
	} //if timezone
	
	else { //no timezone
				
		file_stream = g_file_read(file, NULL, NULL);		
		input_stream = g_data_input_stream_new(G_INPUT_STREAM(file_stream));
					
		char* line2="";
		while ((line2 = g_data_input_stream_read_line (input_stream, NULL, NULL, NULL))) {
	
		
			g_strstrip (line2);	
			
			gchar** tokens = g_strsplit (line2, ":", -1);
			key =tokens[0];
			value =tokens[1];
			
			if (g_strcmp0 (key,"DTSTART") == 0) //date time start version 1 style
	        {
				
				//no timezone
				const char* start_date_str =g_strdup(value);				
				char* start_year_substring="";
				char* start_month_substring="";
				char* start_day_substring="";
				char* start_hour_str="";
				char* start_min_str="";
			
				start_year_substring = g_utf8_substring (start_date_str,0,4);
				start_month_substring = g_utf8_substring (start_date_str,4,6);
				start_day_substring = g_utf8_substring (start_date_str,6,8);
				
				start_hour_str = g_utf8_substring (start_date_str,9,11);
				start_min_str = g_utf8_substring (start_date_str,11,13);
				
				start_month_substring =ignore_first_zero(start_month_substring);
				start_day_substring =ignore_first_zero(start_day_substring);				
				start_hour_str =ignore_first_zero(start_hour_str);
				start_min_str =ignore_first_zero(start_min_str);
				
				if (strcmp (start_hour_str,"") == 0) start_hour_str ="0";
				if (strcmp (start_min_str,"") == 0) start_min_str ="0";
								
				//convert str to int using str-to-long-int
				start_year=g_ascii_strtoll(start_year_substring, NULL, 0);
				start_month=g_ascii_strtoll(start_month_substring, NULL, 0);
				start_day=g_ascii_strtoll(start_day_substring, NULL, 0);
				
				start_hour=g_ascii_strtoll(start_hour_str, NULL, 0);
				start_min=g_ascii_strtoll(start_min_str, NULL, 0);	
	        }	
	        
	        if (g_strcmp0 (key,"DTEND") == 0) //date time end
	        {
				//no timezone
				const char* end_date_str =g_strdup(value);				
				char* end_year_substring="";
				char* end_month_substring="";
				char* end_day_substring="";
				char* end_hour_str="";
				char* end_min_str="";
							
				end_year_substring = g_utf8_substring (end_date_str,0,4);
				end_month_substring = g_utf8_substring (end_date_str,4,6);
				end_day_substring = g_utf8_substring (end_date_str,6,8);
				
				end_hour_str = g_utf8_substring (end_date_str,9,11);
				end_min_str = g_utf8_substring (end_date_str,11,13);
				
				end_month_substring =ignore_first_zero(end_month_substring);
				end_day_substring =ignore_first_zero(end_day_substring);				
				end_hour_str =ignore_first_zero(end_hour_str);
				end_min_str =ignore_first_zero(end_min_str);
				
				if (strcmp (end_hour_str,"") == 0) end_hour_str ="0";
				if (strcmp (end_min_str,"") == 0) end_min_str ="0";
				
				//convert str to int using str-to-long-int
				end_year=g_ascii_strtoll(end_year_substring, NULL, 0);
				end_month=g_ascii_strtoll(end_month_substring, NULL, 0);
				end_day=g_ascii_strtoll(end_day_substring, NULL, 0);				
				end_hour=g_ascii_strtoll(end_hour_str, NULL, 0);
				end_min=g_ascii_strtoll(end_min_str, NULL, 0);
	        }
	        
	         if (g_strcmp0 (key,"SUMMARY") == 0)
			{				
				summary_str=g_strdup(value);
			}
			if (g_strcmp0 (key,"LOCATION") == 0)
	        {				
				location_str=g_strdup(value);
	        }
	        if (g_strcmp0 (key,"DESCRIPTION") == 0)
	        {				
				description_str=g_strdup(value);
	        }
	        
			if (strcmp (key,"PRIORITY") == 0)
	       {
			   const char* priority_str =g_strdup(value);
			   int p_num=g_ascii_strtoll(priority_str, NULL, 0);
			   if (p_num >0) is_priority=1;
			   else is_priority=0;			  
		   }
		   
		   
		   if (strcmp (key,"RRULE") == 0)
		   {
			    const char* rule_str =g_strdup(value);			   			    
			   //YEARLY is the only RRULE recurring event currently supported
			   //More work needed here.			    
			   if (g_strrstr(rule_str,"FREQ=YEARLY") != NULL)
			   {
				   is_yearly=1;
			   }			   
		   }
		   
	        //event ends and so insert into db	        
			if ((strcmp (key,"END") == 0) && (strcmp (value,"VEVENT") == 0)) //VEVENT ends
			{
								
				//is_allday=0;
				//if((start_hour==0) 
				//&& (start_min==0)
				//&& (end_hour==0)
				//&&(end_min==0))
				//{
					//is_allday=1;
				//}
				
				is_allday=1;
				if((start_hour!=0) && (start_min!=0))				
				{
					is_allday=0;
				}	
						
				//VALARM to do
								
				CalendarEvent *evt = g_object_new(CALENDAR_TYPE_EVENT, 0);
				
				g_object_set(evt, "summary", g_strdup(summary_str), NULL);
				g_object_set(evt, "location", g_strdup(location_str), NULL);
				g_object_set(evt, "description", g_strdup(description_str), NULL);
				g_object_set(evt, "startyear", start_year, NULL);
				g_object_set(evt, "startmonth", start_month, NULL);
				g_object_set(evt, "startday", start_day, NULL);
				g_object_set(evt, "starthour", start_hour, NULL);
				g_object_set(evt, "startmin", start_min, NULL);
				g_object_set(evt, "endyear", end_year, NULL); // to do
				g_object_set(evt, "endmonth", end_month, NULL);
				g_object_set(evt, "endday", end_day, NULL);
				g_object_set(evt, "endhour", end_hour, NULL);
				g_object_set(evt, "endmin", end_min, NULL);
				g_object_set(evt, "isyearly", is_yearly, NULL);
				g_object_set(evt, "isallday", is_allday, NULL);			
				g_object_set(evt, "ispriority", is_priority, NULL);			
				
				g_object_set(evt, "hasreminder", has_reminder, NULL); //todo
				g_object_set(evt, "reminderhour",reminder_hour, NULL);
				g_object_set(evt, "remindermin", reminder_min, NULL);
				
				db_insert_event(evt); //insert event into database	
				
				//reset is_yearly for next event
				is_yearly=0;
				is_priority=0;
				is_allday=0;	
				
										
				
			}	//VEVENT END	
			
			g_free(line2);
			
		} //while parse
		
	} //else no timezone
   	
   	
   	m_id_selection = -1; //no selection
    
	set_titles_on_calendar(CUSTOM_CALENDAR(calendar));	
	custom_calendar_update(CUSTOM_CALENDAR(calendar));	
   	 
   	g_object_unref(file);   	
	g_object_unref (input_stream);
	g_object_unref (file_stream);
	return true;
   
}
//=====================================================================

//======================================================================
// GAsync callback
//======================================================================
static void file_chooser_response (GObject *source, GAsyncResult *result, void *user_data)
{
  //GtkWidget *window = user_data;
  GtkFileDialog *dialog = GTK_FILE_DIALOG (source);    
  GtkWidget *window = g_object_get_data(G_OBJECT(dialog), "dialog-window-key");
  
  GFile *file;

  file = gtk_file_dialog_open_finish (dialog, result, NULL);
  if (file)
    {     

	  //import_ical_file(window);	  
      
      char* file_name=g_file_get_path(file);
	 		  
	  gboolean is_success = import_ical_file(file_name,window);
	  
	  //todo dialog if fails
	  //g_print("import calendar: is_success = %d\n",is_success);      
      
      g_object_unref (file);
    }
}   

//======================================================================
// import ical
//======================================================================

static void callbk_import(GSimpleAction *action, GVariant *parameter,  gpointer user_data)
{
	
	GtkWidget *window = user_data;
	GtkFileDialog *dialog; //file dialog
	dialog = gtk_file_dialog_new();
	gtk_file_dialog_set_title (dialog,"Import ical calendar file");  
	
	//iCalendar files typically have the file extension ".ical" ".ics" "
	//.ifb"  or ".icalendar" with a MIME type of "text/calendar
	
	GtkFileFilter *filefilter0 = gtk_file_filter_new();
	gtk_file_filter_set_name(filefilter0,"All Files");
    gtk_file_filter_add_pattern(filefilter0, "*");
	
	GtkFileFilter* filefilter1 = gtk_file_filter_new();
	gtk_file_filter_add_suffix(filefilter1,"ical");
	gtk_file_filter_set_name(filefilter1,"ical");
	
	GtkFileFilter *filefilter2 = gtk_file_filter_new();
	gtk_file_filter_add_suffix(filefilter2,"ics");
	gtk_file_filter_set_name(filefilter2,"ics");
	
	GtkFileFilter *filefilter3 = gtk_file_filter_new();
	gtk_file_filter_add_suffix(filefilter3,"ifb");
	gtk_file_filter_set_name(filefilter3,"ifb");
	
	GtkFileFilter *filefilter4 = gtk_file_filter_new();
	gtk_file_filter_add_suffix(filefilter4,"icalendar");
	gtk_file_filter_set_name(filefilter4,"icalendar");
	
	GtkFileFilter* filefilter5 = gtk_file_filter_new();
	gtk_file_filter_add_suffix(filefilter5,"txt");
	gtk_file_filter_set_name(filefilter5,"Text");
	
	GListStore* liststore = g_list_store_new (GTK_TYPE_FILE_FILTER);
	g_list_store_append(liststore, filefilter0);
	g_list_store_append(liststore, filefilter1);
	g_list_store_append(liststore, filefilter2);
	g_list_store_append(liststore, filefilter3);
	g_list_store_append(liststore, filefilter4);
	g_list_store_append(liststore, filefilter5);
			
	
	gtk_file_dialog_set_filters(dialog,G_LIST_MODEL(liststore));
	gtk_file_dialog_open (dialog, NULL, NULL, file_chooser_response, NULL); //no longer signal based
	
	g_object_set_data(G_OBJECT(dialog), "dialog-window-key",window);	
	g_object_unref (dialog);
	
}


//======================================================================

static void task_callbk(GObject *gobject,GAsyncResult *res,  gpointer  user_data)
{		
	//the task callbk function is called back when the 
	//play_audio_async function has completed
	//m_speaking is reset to false so that we can speak again
		
    m_speaking=FALSE; 
    //g_print("gtask finished reset m_speaking = %d\n",m_speaking);	
    
}



//======================================================================

//======================================================================
static void callbk_about(GSimpleAction * action, GVariant *parameter, gpointer user_data){


	GtkWidget *window = user_data;

	const gchar *authors[] = {"Alan Crispin", NULL};
	GtkWidget *about_dialog;
	about_dialog = gtk_about_dialog_new();
	gtk_window_set_transient_for(GTK_WINDOW(about_dialog),GTK_WINDOW(window));
	gtk_widget_set_size_request(about_dialog, 200,200);
    gtk_window_set_modal(GTK_WINDOW(about_dialog),TRUE);
	gtk_about_dialog_set_program_name(GTK_ABOUT_DIALOG(about_dialog), "Speaking Calendar");
	gtk_about_dialog_set_version (GTK_ABOUT_DIALOG(about_dialog), "Version 0.1.2");
	gtk_about_dialog_set_copyright(GTK_ABOUT_DIALOG(about_dialog),"Copyright © 2024");
	gtk_about_dialog_set_comments(GTK_ABOUT_DIALOG(about_dialog),"Linux Speaking Calendar");
	gtk_about_dialog_set_license_type (GTK_ABOUT_DIALOG(about_dialog), GTK_LICENSE_LGPL_2_1);
	gtk_about_dialog_set_website(GTK_ABOUT_DIALOG(about_dialog),"https://github.com/crispinprojects/");
	gtk_about_dialog_set_website_label(GTK_ABOUT_DIALOG(about_dialog),"Speaking Calendar Website");
	gtk_about_dialog_set_authors(GTK_ABOUT_DIALOG(about_dialog), authors);
	gtk_about_dialog_set_logo_icon_name(GTK_ABOUT_DIALOG(about_dialog), "x-office-calendar");
	gtk_widget_set_visible (about_dialog, TRUE);
}

//======================================================================
// Speaking
//======================================================================
static void speak_reminder()
{
	
	if(m_speak==0) return;
	if (m_speaking ==TRUE) return;
	
	GList *speak_word_list = NULL;	
	speak_word_list = g_list_append(speak_word_list, "calendar");
	speak_word_list = g_list_append(speak_word_list, "reminder");
	
	int word_number  =g_list_length(speak_word_list);
	
	unsigned char *word_arrays[word_number]; 
	unsigned int word_arrays_sizes[word_number];
			
	get_words_array(speak_word_list, word_number,word_arrays,word_arrays_sizes);
		
   //concatenate using raw cat
	unsigned char *data = rawcat(word_arrays, word_arrays_sizes, word_number);	
	unsigned int data_len = get_merge_size(word_arrays_sizes,word_number);
	
	FILE* f = fopen(m_raw_file, "w");
    fwrite(data, data_len, 1, f);
    fclose(f); 
    
	//GTask* task = g_task_new(NULL, NULL, NULL, NULL);
	GTask* task = g_task_new(NULL, NULL, task_callbk, NULL);
    g_task_run_in_thread(task, play_audio_async);     
    g_object_unref(task);
	
	//clean up 
	g_list_free(speak_word_list);	
	free(data);	//prevent memory leak
	
}

//======================================================================
// Speak time
//======================================================================

static void speak_time(gint hour, gint min) 
{	
	if(m_speak==0) return;
	if (m_speaking ==TRUE) return;
	
	gchar* hour_str="";
	gchar* min_str="";
	gchar* ampm_str="";
	GList *speak_word_list = NULL;
		
	if(m_12hour_format) {
		
	if (hour >= 13 && hour <= 23)
	{
	int s_hour = hour - 12;
	ampm_str = "pm";					
	hour_str =get_cardinal_string(s_hour);
	}
	if(hour == 12)
	{
	ampm_str = "pm";					
	hour_str =get_cardinal_string(hour);
	}
	if(hour <12)
	{
	ampm_str = "am";					
	hour_str =get_cardinal_string(hour);
	}
	
	speak_word_list = g_list_append(speak_word_list, hour_str);
	
	if (min > 0 && min < 10)
	{				
	speak_word_list = g_list_append(speak_word_list, "o");
	min_str=get_cardinal_string(min);
	speak_word_list = g_list_append(speak_word_list, min_str);
	}
	else if(min >=10)
	{
	min_str=get_cardinal_string(min);
	speak_word_list = g_list_append(speak_word_list, min_str);
	}
	
	speak_word_list = g_list_append(speak_word_list,ampm_str);
		
	} //12hour format
	
	else
	{				
	hour_str =get_cardinal_string(hour);	
	speak_word_list = g_list_append(speak_word_list, hour_str);
	
	if (min > 0 && min < 10)
	{				
	speak_word_list = g_list_append(speak_word_list, "o");
	min_str=get_cardinal_string(min);
	speak_word_list = g_list_append(speak_word_list, min_str);
	}
	else if(min >=10)
	{
	min_str=get_cardinal_string(min);
	speak_word_list = g_list_append(speak_word_list, min_str);
	}			    				
	} //24 hour format
				
		
	int word_number  =g_list_length(speak_word_list);
	
	unsigned char *word_arrays[word_number]; 
	unsigned int word_arrays_sizes[word_number];
			
	get_words_array(speak_word_list, word_number,word_arrays,word_arrays_sizes);
	
	//concatenate using raw cat
	unsigned char *data = rawcat(word_arrays, word_arrays_sizes, word_number);	
	unsigned int data_len = get_merge_size(word_arrays_sizes,word_number);
	
    FILE* f = fopen(m_raw_file, "w");
    fwrite(data, data_len, 1, f);
    fclose(f); 
    
    GTask* task = g_task_new(NULL, NULL, task_callbk, NULL);
	//GTask* task = g_task_new(NULL, NULL, NULL, NULL);
    g_task_run_in_thread(task, play_audio_async);     
    g_object_unref(task);
	
	//clean up 
	g_list_free(speak_word_list);	
	free(data);	//prevent memory leak
	
}
//======================================================================

static void callbk_speaktime(GSimpleAction * action, GVariant *parameter, gpointer user_data)
{	
	GtkWidget *window = user_data;
	
	GDateTime *dt = g_date_time_new_now_local(); 
	gint hour =g_date_time_get_hour(dt);	
	gint min= g_date_time_get_minute(dt);	
	
	if(m_speaking==FALSE) speak_time(hour,min);
	
			
    g_date_time_unref (dt);
}

//=====================================================================

static void callbk_speak(GSimpleAction* action, GVariant *parameter,gpointer user_data)
{	
	if(m_speaking == FALSE) speak_events();	
}
//======================================================================

//======================================================================
static void play_audio_async (GTask *task,
                          gpointer object,
                          gpointer task_data,
                          GCancellable *cancellable)
{
   
    m_speaking=TRUE; //stop any new speaking 
          
    gchar *m_sample_rate_str = g_strdup_printf("%i", m_speak_rate); 
    gchar *sample_rate_str ="-r ";    
    sample_rate_str= g_strconcat(sample_rate_str,m_sample_rate_str, NULL);     
    //gchar * command_str ="aplay -c 1 -f S16_LE";
    gchar * command_str ="aplay -c 1 -f U8";
    command_str =g_strconcat(command_str," ",sample_rate_str, " ", m_raw_file, NULL);     
    system(command_str);   
   
    //m_speaking=FALSE;   
    g_task_return_boolean(task, TRUE);
}


//======================================================================
// Concatentation
//======================================================================

//-----------------------------------------------------------------
// Speak functions and callbks
//------------------------------------------------------------------


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

unsigned int get_merge_size(unsigned int sizes_arry[], int arry_size){
	
	unsigned int total_samples=0;
	for (int i = 0; i < arry_size; i++) 
	{  
    unsigned int count =sizes_arry[i]; 
    total_samples=total_samples+count;	
    }
	return total_samples;
}
//=====================================================================
static char* get_day_of_week(int day, int month, int year) 
{

	char* weekday_str="";
	GDate* day_date;
	day_date = g_date_new_dmy(day, month, year);
	GDateWeekday weekday =g_date_get_weekday(day_date);
		
		
	switch(weekday)
	{
	case G_DATE_MONDAY:
	weekday_str="monday";
	break;
	case G_DATE_TUESDAY:
	weekday_str="tuesday";
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
//---------------------------------------------------------------------
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
//=====================================================================

//char* get_day_string(int day) {

	//char* result ="";
	
	//switch(month) {
	//case 1:
		//result = "monday";
		//break;
	//case 2:
		//result = "tuesday";
		//break;
	//case 3:
		//result= "wednesday";
		//break;
	//case 4:
		//result = "thursday";
		//break;
	//case 5:
		//result ="friday";
		//break;
	//case 6:
		//result = "saturday";
		//break;
	//case 7:
		//result ="sunday";
		//break;
	
	//default:
		//result = "unknown";
	//}
	//return result;
//}

//=====================================================================

//static char* get_event_title_word(char* word) 
//{

//char* result="";

	//if (g_strcmp0(word,"activity")==0) {
	//result ="activity";
	//}	
	//if (g_strcmp0(word,"anniversary")==0) {
	//result="anniversary";
	//}
	//if (g_strcmp0(word,"appointment")==0) {
	//result="appointment";
	//}	
	//if (g_strcmp0(word,"birthday")==0) {
	//result="birthday";
	//}
	//if (g_strcmp0(word,"cabbie")==0) {
	//result="cabbie";
	//}
	//if (g_strcmp0(word,"cafe")==0) {
	//result="cafe";
	//}
	//if (g_strcmp0(word,"car")==0) {
	//result="car";
	//}
	////dwords
	//if (g_strcmp0(word,"dentist")==0) {
	//result="dentist";
	//}
	//if (g_strcmp0(word,"doctor")==0) {
	//result="doctor";
	//}
	////f words
	//if (g_strcmp0(word,"family")==0) {
	//result ="family";
	//}
	//if (g_strcmp0(word,"funeral")==0) {
	//result ="funeral";
	//}		
	//if (g_strcmp0(word,"holiday")==0) {
	//result="holiday";
	//}
	//if (g_strcmp0(word,"hospital")==0) {
	//result="hospital";
	//}
	
	//if (g_strcmp0(word,"meeting")==0) {
	//result="meeting";
	//}
	//if (g_strcmp0(word,"meetup")==0) {
	//result="meetup";
	//}
	////pwords
	//if (g_strcmp0(word,"payment")==0) {
	//result="payment";
	//}
	////rwords
	//if (g_strcmp0(word,"reminder")==0) {
	//result="reminder";
	//}
	//if (g_strcmp0(word,"restaurant")==0) {
	//result="restaurant";
	//}
	//// s words
	//// t words
	//if (g_strcmp0(word,"task")==0) {
	//result="task";
	//}
	//if (g_strcmp0(word,"travel")==0) {
	//result="travel";
	//}
	////vwords
	//if (g_strcmp0(word,"visit")==0) {
	//result="visit";
	//}		
	//if (g_strcmp0(word,"work")==0) {
	//result="work";
	//}

//return result;
//}

//----------------------------------------------------------------------
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

GList *get_public_holiday_speak_list()
{
	GList *holiday_word_list = NULL;
		
	if (m_start_month==1 && m_start_day ==1) {
	holiday_word_list = g_list_append(holiday_word_list, "new");
	holiday_word_list = g_list_append(holiday_word_list, "year");
	}

	if (m_start_month==12 && m_start_day==25) {
	//christmas day
	holiday_word_list = g_list_append(holiday_word_list, "christmas");
	holiday_word_list = g_list_append(holiday_word_list, "day");
	}

	if (m_start_month==12 && m_start_day==26) {
	//boxing day
	holiday_word_list = g_list_append(holiday_word_list, "boxing");
	holiday_word_list = g_list_append(holiday_word_list, "day");
	}
	//Easter		
	GDate *easter_date =calculate_easter(m_start_year);
	int easter_day = g_date_get_day(easter_date);
	int easter_month =g_date_get_month(easter_date);

	if(m_start_month==easter_month && m_start_day == easter_day)
	{
	//easter day
	holiday_word_list = g_list_append(holiday_word_list, "easter");
	holiday_word_list = g_list_append(holiday_word_list, "day");
	}
	
	g_date_subtract_days(easter_date,2);
	int easter_friday = g_date_get_day(easter_date);
	int easter_friday_month =g_date_get_month(easter_date);

	if(m_start_month==easter_friday_month && m_start_day ==easter_friday)
	{
	//easter friday
	holiday_word_list = g_list_append(holiday_word_list, "easter");
	holiday_word_list = g_list_append(holiday_word_list, "friday");
	}
	g_date_add_days(easter_date,3);
	int easter_monday = g_date_get_day(easter_date); //easter monday
	int easter_monday_month =g_date_get_month(easter_date);

	if(m_start_month==easter_monday_month && m_start_day ==easter_monday)
	{
	//easter monday
	holiday_word_list = g_list_append(holiday_word_list, "easter");
	holiday_word_list = g_list_append(holiday_word_list, "monday");
	}	
	
	//May complicated
	if (m_start_month == 5) {
     GDate *first_monday_may;
     first_monday_may = g_date_new_dmy(1, m_start_month, m_start_year);

     while (g_date_get_weekday(first_monday_may) != G_DATE_MONDAY)
       g_date_add_days(first_monday_may,1);

     int may_day = g_date_get_day(first_monday_may);

     if( m_start_day==may_day) {		
		 holiday_word_list = g_list_append(holiday_word_list, "bank");
	     holiday_word_list = g_list_append(holiday_word_list, "holiday");
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
	 holiday_word_list = g_list_append(holiday_word_list, "spring");
     holiday_word_list = g_list_append(holiday_word_list, "bank");
     holiday_word_list = g_list_append(holiday_word_list, "holiday");     
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
	holiday_word_list = g_list_append(holiday_word_list, "bank");
	holiday_word_list = g_list_append(holiday_word_list, "holiday"); 
	}

    } //m_start_month==8 (august)
	
	return holiday_word_list;
}
//======================================================================
// speak events
//======================================================================
static void speak_events() {

	if(m_speak==0) return;
	if (m_speaking ==TRUE) return;
	
	//GList *speak_word_list = NULL;
	g_autoptr(GList) speak_word_list=NULL;
	//---------------------------------------------------------------
	
	gchar *dow_str=get_day_of_week(m_start_day, m_start_month, m_start_year);	//get day of week
	
	
	gchar *day_number_str=get_day_number_ordinal_string(m_start_day); //get day number
	gchar *month_str=get_month_string(m_start_month); //get month
	
	speak_word_list = g_list_append(speak_word_list, dow_str);
	speak_word_list = g_list_append(speak_word_list, day_number_str);
	speak_word_list = g_list_append(speak_word_list, month_str);
		
	
	//Check for public holidays and special days (Christmas, Easter, Fathers etc,.)
	//---------------------------------------------------------------
	if ((m_holidays ==1) && (is_public_holiday(m_start_day)))	
	{
		//markup public holidays
		gpointer word_list_pointer;
	    gchar* word_str;  
		
		GList *holiday_word_list = get_public_holiday_speak_list();
		
		for (int i =0; i < g_list_length(holiday_word_list); i++){		
			word_list_pointer=g_list_nth_data(holiday_word_list,i);
		    word_str=(gchar *)word_list_pointer;
		    //gchar* word_str_lower= g_ascii_strdown(word_str,-1);
		    speak_word_list = g_list_append(speak_word_list, word_str);			
		}	
		g_list_free(holiday_word_list);	
	} //if public holidays 
	
	//cycle through day events adding event titles
	GArray *day_events_arry =g_array_new(FALSE, FALSE, sizeof(CALENDAR_TYPE_EVENT));
	db_get_all_events_year_month_day(day_events_arry , m_start_year,m_start_month, m_start_day);
	int event_count = day_events_arry->len;
	
	//Speak event number if required
	
	if(m_speak_event_number) {
	int event_number = day_events_arry->len;		
	if (event_number ==0) {			
	speak_word_list = g_list_append(speak_word_list, "no");
	speak_word_list = g_list_append(speak_word_list, "events");			
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
	} //m_speak_event_number
	
	//Speak time and event word if required	
	for (int i = 0; i < day_events_arry->len; i++)
	{
		gint evt_id = 0;
		gchar *summary_str = "";
		gchar *event_number_str="";
		gint start_hour = 0;
		gint start_min = 0;
		gint is_allday = 0;
		gint is_priority = 0;
		
		CalendarEvent *evt = g_array_index(day_events_arry, CalendarEvent *, i);
		
		g_object_get(evt, "summary", &summary_str, NULL);		
		g_object_get(evt, "starthour", &start_hour, NULL);
		g_object_get(evt, "startmin", &start_min, NULL);		
		g_object_get(evt, "isallday", &is_allday, NULL);
		g_object_get(evt, "ispriority", &is_priority, NULL);		
		
		//Get time first
		gchar* hour_str="";
		gchar* min_str="";
		gchar* ampm_str="";
		
		if(m_speak_time) {
		
		if(!is_allday) {		
		
		if(m_12hour_format) {
		
		if (start_hour >= 13 && start_hour <= 23)
		{
		int s_hour = start_hour - 12;
		ampm_str = "pm";					
		hour_str =get_cardinal_string(s_hour);
		}
		if(start_hour == 12)
		{
		ampm_str = "pm";					
		hour_str =get_cardinal_string(start_hour);
		}
		if(start_hour <12)
		{
		ampm_str = "am";					
		hour_str =get_cardinal_string(start_hour);
		}
		
		speak_word_list = g_list_append(speak_word_list, hour_str);
		
		if (start_min > 0 && start_min< 10)
		{				
		speak_word_list = g_list_append(speak_word_list, "o");
		min_str=get_cardinal_string(start_min);
		speak_word_list = g_list_append(speak_word_list, min_str);
		}
		else if(start_min >=10)
		{
		min_str=get_cardinal_string(start_min);
		speak_word_list = g_list_append(speak_word_list, min_str);
		}
		
		speak_word_list = g_list_append(speak_word_list,ampm_str);
		
		} //12hour format
		
		else
		{				
		hour_str =get_cardinal_string(start_hour);	
		speak_word_list = g_list_append(speak_word_list, hour_str);
		
		if (start_min > 0 && start_min < 10)
		{				
		speak_word_list = g_list_append(speak_word_list, "o");
		min_str=get_cardinal_string(start_min);
		speak_word_list = g_list_append(speak_word_list, min_str);
		}
		else if(start_min >=10)
		{
		min_str=get_cardinal_string(start_min);
		speak_word_list = g_list_append(speak_word_list, min_str);
		}			    				
		} //24 hour format
				
		} //not allday	
		}//m_speak_time
	
		
		//now add event title
		if(m_speak_event_words) {
		gchar* summary_lower= g_ascii_strdown(summary_str,-1);		
		//gchar *event_title =get_event_title_word(summary_lower);		
		speak_word_list = g_list_append(speak_word_list, summary_str);
		}//m_speak_event_words	
		
		if(is_priority) {
		speak_word_list = g_list_append(speak_word_list, "high");
		speak_word_list = g_list_append(speak_word_list, "priority");
		}		
		
		speak_word_list = g_list_append(speak_word_list, "space"); //space between events
		
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
		if(m_speak_upcoming && m_start_day==today && m_start_month ==month && m_start_year==year)  
		{		
			
			GArray *evts_upcoming = get_upcoming_array(m_upcoming_days); //next days		
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
			 speak_word_list = g_list_append(speak_word_list, "space"); //space between events	
			 speak_word_list = g_list_append(speak_word_list, "space");			
			}
					
		}//m_speak_upcoming	
	
	
	gint word_number  =g_list_length(speak_word_list);
		
	//create word array using list size
	unsigned char *word_arrays[word_number]; 
	unsigned int word_arrays_sizes[word_number];
	//use word dictioary
	get_words_array(speak_word_list, word_number,word_arrays,word_arrays_sizes);
		
	//concatenate using raw cat
	unsigned char *data = rawcat(word_arrays, word_arrays_sizes, word_number);	
	unsigned int data_len = get_merge_size(word_arrays_sizes,word_number);	
    
    FILE* f = fopen(m_raw_file, "w");
    fwrite(data, data_len, 1, f);
    fclose(f); 
    
	GTask* task = g_task_new(NULL, NULL, task_callbk, NULL);
    g_task_run_in_thread(task, play_audio_async);     
    g_object_unref(task);
	
	//clean up 
	//g_list_free(speak_word_list);	//now an auto pointer
	free(data);	//prevent memory leak	
}

//=====================================================================
void dialog_search_shutdown(GtkWindow *dialog, gint response_id, gpointer user_data)
{	
	gtk_window_destroy(GTK_WINDOW(dialog));	
}


//=====================================================================
static void search_events(const char* search_str)
{
	
	char* search_str_lower = g_ascii_strdown(search_str,-1);
	//cycle through all events for text str
	GArray *all_events_arry =g_array_new(FALSE, FALSE, sizeof(CALENDAR_TYPE_EVENT));
	db_get_all_events(all_events_arry);
		
	GArray *search_events_arry;
	search_events_arry = g_array_new(FALSE, FALSE, sizeof(CALENDAR_TYPE_EVENT)); 
	
	//find search events array
	for (int i = 0; i < all_events_arry->len; i++)
	{
		gint evt_id = 0;
		gchar *summary_str = "";
		gchar *description_str = "";		
		gchar *location_str = "";
				
		//get each event
		CalendarEvent *evt = g_array_index(all_events_arry, CalendarEvent *, i);
		
		g_object_get(evt, "summary", &summary_str, NULL);
		g_object_get(evt, "description", &description_str, NULL);
		g_object_get(evt, "location", &location_str, NULL);		
		
		char* summary_str_lower= g_ascii_strdown(summary_str,-1);
		char* description_str_lower= g_ascii_strdown(description_str,-1);		
		char* location_str_lower= g_ascii_strdown(location_str,-1);	
		//contains substring
		
		char *event_str="";
		
		event_str= g_strconcat(event_str,summary_str_lower, " ",description_str_lower," ", location_str_lower, NULL);	
		
		char * result;
		//result = strstr (summary_str_lower,search_str_lower);
		result = strstr (event_str,search_str_lower);
		if (result != NULL)
		{
		g_array_append_val(search_events_arry, evt);
		}
    } //all events
      	
	GtkWidget *dialog_search_results;		
	//GtkWidget *label;
	GtkWidget *listbox, *box, *label;
	
	dialog_search_results =gtk_window_new(); 
	gtk_window_set_title (GTK_WINDOW (dialog_search_results), "Search Results");
	gtk_window_set_default_size(GTK_WINDOW(dialog_search_results),400,300);
	//g_signal_connect (GTK_WINDOW(dialog_search_results), "destroy", G_CALLBACK (dialog_search_shutdown), NULL);
	
	listbox = gtk_list_box_new ();
	gtk_list_box_set_selection_mode(GTK_LIST_BOX(listbox), GTK_SELECTION_SINGLE);
	//g_signal_connect (listbox, "row-activated", G_CALLBACK (callbk_row_activated),NULL);
		
	for (int i = 0; i < search_events_arry->len; i++)
	{
	CalendarEvent *evt_day = g_array_index(search_events_arry, CalendarEvent *, i);
	
	int start_day=0;
	int start_month=0;
	int start_year=0;
	char* summary_str="";	
	char *location_str="";
	char *description_str="";	
	int start_hour=0;
	int start_min=0;	
	int end_hour=0;
	int end_min=0;
	int is_yearly=0;
	int is_allday=0;
	int is_priority=0;
		
	g_object_get (evt_day, "startday", &start_day, NULL);
	g_object_get (evt_day, "startmonth", &start_month, NULL);
	g_object_get (evt_day, "startyear", &start_year, NULL);	
	g_object_get(evt_day, "summary", &summary_str, NULL);	
	g_object_get(evt_day, "location", &location_str, NULL);
	g_object_get(evt_day, "description", &description_str, NULL);	
	g_object_get(evt_day, "starthour", &start_hour, NULL);
	g_object_get(evt_day, "startmin", &start_min, NULL);	
	g_object_get(evt_day, "endhour", &end_hour, NULL);
	g_object_get(evt_day, "endmin", &end_min, NULL);
	g_object_get(evt_day, "isyearly", &is_yearly, NULL);
	g_object_get(evt_day, "isallday", &is_allday, NULL);
	g_object_get(evt_day, "ispriority", &is_priority, NULL);
		
	char *display_str="";
	
	char *day_str= g_strdup_printf("%d",start_day);
	char *month_str= g_strdup_printf("%d",start_month);
	char *year_str= g_strdup_printf("%d",start_year);
	
	display_str = g_strconcat(display_str, "Date: ",day_str,"-",month_str,"-",year_str, "\n", NULL);
	
	char *time_str = "";
	char *starthour_str = "";
	char *startmin_str = "";
	char *endhour_str = "";
	char *endmin_str = "";
	char *ampm_str = " ";
		
	if(!is_allday)
	{
	//if not all_day then add start time
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
	
	time_str = g_strconcat(time_str, NULL);	
	display_str = g_strconcat(display_str, time_str, summary_str, "\n", NULL);	
		
	}//if not allday
	else
	{
		display_str=g_strconcat(display_str,summary_str, "\n",NULL);
	}
	
	//if (is_priority)
	//{
	//display_str = g_strconcat(display_str, " High Priority.", NULL);
	//}
	
	if (!strlen(description_str) == 0){
	display_str = g_strconcat(display_str,description_str, "\n", NULL);
	}
	
	if (!strlen(location_str) == 0){
	display_str = g_strconcat(display_str,location_str, "\n", NULL);
	}
		
	box = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 6);
	gtk_list_box_append (GTK_LIST_BOX (listbox), box);	
	label = gtk_label_new (display_str);
	gtk_box_append (GTK_BOX (box), label);	
    
    }//for searchevents
	
	gtk_window_set_child (GTK_WINDOW (dialog_search_results),listbox);
	gtk_window_present (GTK_WINDOW (dialog_search_results));
       	
	g_array_free(all_events_arry, FALSE); //clear the array
	g_array_free(search_events_arry, FALSE); //clear the array	

}


//======================================================================

static void callbk_search_events(GtkButton *button, gpointer user_data)
{

	GtkWidget *window = user_data;
	
	GtkEntryBuffer *buffer_search;
	GtkWidget *entry_search = g_object_get_data(G_OBJECT(button), "entry-search-key");
	
	buffer_search = gtk_entry_get_buffer(GTK_ENTRY(entry_search));
	
	const char* search_str=gtk_entry_buffer_get_text(buffer_search);
	
	//search_str = gtk_entry_buffer_get_text(search_str);
	search_str = remove_semicolons(search_str);
	search_str = remove_commas(search_str);
	search_str =remove_punctuations(search_str);
	//g_print("search_events: search_str = %s\n", search_str);

	search_events(search_str);
}

//=====================================================================

static void callbk_search(GSimpleAction *action, GVariant *parameter,  gpointer user_data)
{
	//search_events("dentist");
	
	GtkWidget *window = user_data;

	GtkWidget *dialog_search;
	gint response;
	
	GtkWidget *box;
	GtkWidget *button_search;
	
	GtkWidget *label_entry_search;
	GtkWidget *entry_search;
	
	dialog_search = gtk_window_new(); 
	gtk_window_set_title(GTK_WINDOW(dialog_search), "Search Events");
	gtk_window_set_default_size(GTK_WINDOW(dialog_search), 300, 100);
	box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 1);
	gtk_window_set_child(GTK_WINDOW(dialog_search), box);

	label_entry_search = gtk_label_new("Search Text: ");
	entry_search = gtk_entry_new();
	gtk_entry_set_max_length(GTK_ENTRY(entry_search), 100);
	
	button_search = gtk_button_new_with_label("Search");
	g_signal_connect(button_search, "clicked", G_CALLBACK(callbk_search_events), window);
	
	g_object_set_data(G_OBJECT(button_search), "dialog-search-key", dialog_search);
	g_object_set_data(G_OBJECT(button_search), "entry-search-key", entry_search);
	
	gtk_box_append(GTK_BOX(box), label_entry_search);
	gtk_box_append(GTK_BOX(box), entry_search);
	
	gtk_box_append(GTK_BOX(box), button_search);	
	gtk_window_present(GTK_WINDOW(dialog_search));	
}

//======================================================================
static void callbk_check_button_upcoming_toggled(GtkCheckButton *check_button, gpointer user_data)
{
	GtkWidget *spin_button_upcoming_days;
	spin_button_upcoming_days = g_object_get_data(G_OBJECT(user_data), "cb_upcoming_spin_upcoming_key");
	
	if (gtk_check_button_get_active(GTK_CHECK_BUTTON(check_button)))
	{
		gtk_widget_set_sensitive(spin_button_upcoming_days, TRUE);
		
	}
	else
	{
		gtk_widget_set_sensitive(spin_button_upcoming_days, FALSE);
	}
}
//======================================================================

//======================================================================
static void callbk_set_preferences(GtkButton *button, gpointer  user_data)
{

    GtkWidget *window = user_data;
	GtkWidget *calendar =g_object_get_data(G_OBJECT(window), "window-calendar-key");
	GtkWidget *label_date =g_object_get_data(G_OBJECT(window), "window-label-date-key");
	GtkWidget *dialog = g_object_get_data(G_OBJECT(button), "dialog-key");

	//calendar
	GtkWidget *check_button_hour_format= g_object_get_data(G_OBJECT(button), "check-button-hour-format-key");
	GtkWidget *check_button_show_end_time= g_object_get_data(G_OBJECT(button), "check-button-show-end-time-key");
	GtkWidget *check_button_holidays= g_object_get_data(G_OBJECT(button), "check-button-holidays-key");
	GtkWidget *check_button_show_tooltips= g_object_get_data(G_OBJECT(button), "check-button-show-tooltips-key");
		
	GtkWidget *colour_button_today= g_object_get_data(G_OBJECT(button), "colour-button-today-key");
	GtkWidget *colour_button_event= g_object_get_data(G_OBJECT(button), "colour-button-event-key");
	GtkWidget *colour_button_holiday= g_object_get_data(G_OBJECT(button), "colour-button-holiday-key");
		
	const GdkRGBA *rgba_today;
	rgba_today = gtk_color_dialog_button_get_rgba(GTK_COLOR_DIALOG_BUTTON(colour_button_today));	
	m_todaycolour = gdk_rgba_to_string (rgba_today);			
		
	const GdkRGBA *rgba_event;
	rgba_event = gtk_color_dialog_button_get_rgba(GTK_COLOR_DIALOG_BUTTON(colour_button_event));	
	m_eventcolour = gdk_rgba_to_string (rgba_event);			
		
	const GdkRGBA *rgba_holiday;
	rgba_holiday = gtk_color_dialog_button_get_rgba(GTK_COLOR_DIALOG_BUTTON(colour_button_holiday));	
	m_holidaycolour = gdk_rgba_to_string (rgba_holiday);			
			
	//speaking
	
	GtkWidget *check_button_speak= g_object_get_data(G_OBJECT(button), "check-button-speak-key");
    GtkWidget *check_button_speak_startup= g_object_get_data(G_OBJECT(button), "check-button-speak-startup-key");
    GtkWidget *check_button_speak_upcoming= g_object_get_data(G_OBJECT(button), "check-button-speak-upcoming-key");
    GtkWidget *check_button_speak_event_number= g_object_get_data(G_OBJECT(button), "check-button-speak-event-number-key");
	GtkWidget *check_button_speak_time= g_object_get_data(G_OBJECT(button), "check-button-speak-time-key");
	GtkWidget *check_button_speak_event_words=g_object_get_data(G_OBJECT(button), "check-button-speak-event-words-key");	
	GtkWidget *spin_button_speak_rate = g_object_get_data(G_OBJECT(button), "spin-speak-rate-key");	
	
	GtkWidget *spin_button_upcoming_days = g_object_get_data(G_OBJECT(button), "spin-upcoming-days-key");
		
    GtkWidget *check_button_reset_all= g_object_get_data(G_OBJECT(button), "check-button-reset-all-key");
	//calendar
	m_12hour_format=gtk_check_button_get_active (GTK_CHECK_BUTTON(check_button_hour_format));
	m_show_end_time=gtk_check_button_get_active (GTK_CHECK_BUTTON(check_button_show_end_time));
	m_holidays=gtk_check_button_get_active (GTK_CHECK_BUTTON(check_button_holidays));
	m_show_tooltips=gtk_check_button_get_active(GTK_CHECK_BUTTON(check_button_show_tooltips));
	
	//speak
	m_speak=gtk_check_button_get_active (GTK_CHECK_BUTTON(check_button_speak));
	m_speak_at_startup=gtk_check_button_get_active (GTK_CHECK_BUTTON(check_button_speak_startup));
	m_speak_upcoming =gtk_check_button_get_active (GTK_CHECK_BUTTON(check_button_speak_upcoming));
	m_speak_event_number=gtk_check_button_get_active (GTK_CHECK_BUTTON(check_button_speak_event_number));
	m_speak_time =gtk_check_button_get_active (GTK_CHECK_BUTTON(check_button_speak_time));
	m_speak_event_words =gtk_check_button_get_active (GTK_CHECK_BUTTON(check_button_speak_event_words));
			
	//capture these in case typed
	m_speak_rate = gtk_spin_button_get_value(GTK_SPIN_BUTTON(spin_button_speak_rate));	
	m_upcoming_days = gtk_spin_button_get_value(GTK_SPIN_BUTTON(spin_button_upcoming_days));
		
	m_reset_preferences=gtk_check_button_get_active(GTK_CHECK_BUTTON(check_button_reset_all));

	if(m_reset_preferences) {
	//reset everything
	//calendar
	m_12hour_format=1;
	m_show_end_time=0;
	m_holidays=0;
	m_show_tooltips=1;
	m_todaycolour="rgb(173,216,230)";
	m_eventcolour="rgb(222,184,135)";
	m_holidaycolour="rgb(102,205,170)";
	//speaking
	m_speak=1;	
	m_speak_rate=7000;
	m_speak_event_words=1;
	m_speak_time=1;	
	m_speak_at_startup=0;
	m_speak_upcoming=0;
	m_speak_event_number=1;
	
	m_reset_preferences=0; //toggle
	}
	
	config_write();	//save preferences
		
	g_object_set(calendar, "todaycolour", m_todaycolour, NULL);
	g_object_set(calendar, "eventcolour", m_eventcolour, NULL);
	g_object_set(calendar, "holidaycolour", m_holidaycolour, NULL);
		
	g_object_set(calendar, "showtooltips", m_show_tooltips, NULL);
	
	set_titles_on_calendar(CUSTOM_CALENDAR(calendar));
	custom_calendar_update(CUSTOM_CALENDAR(calendar));
		
	//g_free(rgba_today);
	//g_free(rgba_event);
	//g_free(rgba_holiday);
	
	gtk_window_destroy(GTK_WINDOW(dialog));
	
}

//======================================================================

static void callbk_preferences(GSimpleAction* action, GVariant *parameter,gpointer user_data)
{	
	GtkWidget *window =user_data;
	
	GtkWidget *dialog;
	GtkWidget *grid;
	
	//Check buttons
	//calendar	
	GtkWidget *check_button_hour_format;
	GtkWidget *check_button_show_end_time;
	GtkWidget *check_button_holidays;
	GtkWidget *check_button_show_tooltips;
	
	//speak
	GtkWidget *check_button_speak;
	GtkWidget *check_button_speak_startup;
	GtkWidget *check_button_speak_upcoming;
	GtkWidget *check_button_speak_event_number;
	GtkWidget *check_button_speak_time;
	GtkWidget *check_button_speak_event_words;	
	
	
	GtkWidget *label_speak_rate;
	GtkWidget *spin_button_speak_rate;

	GtkWidget *label_upcoming_days;
	GtkWidget *spin_button_upcoming_days;
		
	GtkWidget *check_button_reset_all;		
	GtkWidget *button_set;	
	
	GtkWidget *label_spacer1;
	GtkWidget *label_spacer2;
	GtkWidget *label_spacer3;
	GtkWidget *label_spacer4;
	GtkWidget *label_spacer5;
	
	label_spacer1 = gtk_label_new("");
	label_spacer2 = gtk_label_new("");
	label_spacer3 = gtk_label_new("");
	label_spacer4 = gtk_label_new("");
	label_spacer5 = gtk_label_new("");
	
	dialog =gtk_window_new(); 
	gtk_window_set_title (GTK_WINDOW (dialog), "Preferences");
	gtk_window_set_modal(GTK_WINDOW (dialog),TRUE);
	gtk_window_set_transient_for(GTK_WINDOW (dialog),GTK_WINDOW(window));
	
	grid = gtk_grid_new();	
	gtk_grid_set_column_homogeneous(GTK_GRID(grid), TRUE);
	
	
	GtkWidget *label_todaycolour;
	GtkWidget *colour_button_today= gtk_color_dialog_button_new(gtk_color_dialog_new());
	GdkRGBA  rgba_today;	
	if (gdk_rgba_parse (&rgba_today,m_todaycolour))
	{
		gtk_color_dialog_button_set_rgba (GTK_COLOR_DIALOG_BUTTON(colour_button_today),&rgba_today);
	}
			
	GtkWidget *label_eventcolour;	
	GtkWidget *colour_button_event= gtk_color_dialog_button_new(gtk_color_dialog_new());	
	GdkRGBA  rgba_event;	
	if (gdk_rgba_parse (&rgba_event,m_eventcolour))
	{
		gtk_color_dialog_button_set_rgba (GTK_COLOR_DIALOG_BUTTON(colour_button_event),&rgba_event);
	}
		
	GtkWidget *label_holidaycolour;	
	GtkWidget *colour_button_holiday= gtk_color_dialog_button_new(gtk_color_dialog_new());
	GdkRGBA  rgba_holiday;	
	if (gdk_rgba_parse (&rgba_holiday,m_holidaycolour))
	{
		gtk_color_dialog_button_set_rgba (GTK_COLOR_DIALOG_BUTTON(colour_button_holiday),&rgba_holiday);
	}
	
	button_set = gtk_button_new_with_label ("Set Preferences");
	g_signal_connect (button_set, "clicked", G_CALLBACK (callbk_set_preferences), window);

	//calendar
	check_button_hour_format = gtk_check_button_new_with_label ("12 Hour Format");
	check_button_show_end_time= gtk_check_button_new_with_label ("Show End Time");
	check_button_holidays = gtk_check_button_new_with_label ("Show Public Holidays");
	check_button_show_tooltips = gtk_check_button_new_with_label ("Show Calendar Tooltips");
	
		
	label_todaycolour = gtk_label_new("Today Colour: ");		
	label_holidaycolour = gtk_label_new("Public Holiday Colour: ");	
	label_eventcolour = gtk_label_new("Event Colour: ");
		
	//speech
	check_button_speak = gtk_check_button_new_with_label ("Enable Speaking");
	check_button_speak_startup = gtk_check_button_new_with_label ("Speak At Startup");
	check_button_speak_upcoming= gtk_check_button_new_with_label ("Speak Upcoming");
	check_button_speak_event_number = gtk_check_button_new_with_label ("Speak Event Number");
	check_button_speak_time= gtk_check_button_new_with_label ("Speak Time");
	check_button_speak_event_words= gtk_check_button_new_with_label ("Speak Event Words");
		
	check_button_reset_all = gtk_check_button_new_with_label ("Reset All");
	
	//sample rate
	GtkAdjustment *adjustment_speech_rate;
	// value,lower,upper,step_increment,page_increment,page_size	
	adjustment_speech_rate = gtk_adjustment_new(7000.00, 5000.00, 20000.00, 1000.0, 1000.0, 0.0);
	// start time spin
	label_speak_rate = gtk_label_new("Speech Rate ");
	spin_button_speak_rate = gtk_spin_button_new(adjustment_speech_rate, 7000, 0);
	gtk_spin_button_set_value(GTK_SPIN_BUTTON(spin_button_speak_rate), m_speak_rate);
	
	//upcoming days
	GtkAdjustment *adjustment_upcoming_days;
	// value,lower,upper,step_increment,page_increment,page_size
	adjustment_upcoming_days = gtk_adjustment_new(7.00, 1.00, 14.00, 1.0, 1.0, 0.0);
	//upcoming days selection (up to 14 upcoming days)
	label_upcoming_days = gtk_label_new("Upcoming days:  ");
	spin_button_upcoming_days = gtk_spin_button_new(adjustment_upcoming_days, 7, 0);
	gtk_spin_button_set_value(GTK_SPIN_BUTTON(spin_button_upcoming_days), m_upcoming_days);
	
	check_button_speak_upcoming= gtk_check_button_new_with_label ("Speak Upcoming Events");
	gtk_check_button_set_active (GTK_CHECK_BUTTON(check_button_speak_upcoming), m_speak_upcoming);
	
	if(m_speak_upcoming) gtk_widget_set_sensitive(spin_button_upcoming_days, TRUE);
	else gtk_widget_set_sensitive(spin_button_upcoming_days, FALSE);
	
	g_signal_connect_swapped(GTK_CHECK_BUTTON(check_button_speak_upcoming), "toggled",	
							 G_CALLBACK(callbk_check_button_upcoming_toggled), check_button_speak_upcoming);	
	g_object_set_data(G_OBJECT(check_button_speak_upcoming), "cb_upcoming_spin_upcoming_key",spin_button_upcoming_days);
						 
	//set calendar preferences
	gtk_check_button_set_active (GTK_CHECK_BUTTON(check_button_hour_format),m_12hour_format);
	gtk_check_button_set_active (GTK_CHECK_BUTTON(check_button_show_end_time), m_show_end_time);	
	gtk_check_button_set_active (GTK_CHECK_BUTTON(check_button_holidays),m_holidays);	
	gtk_check_button_set_active (GTK_CHECK_BUTTON(check_button_show_tooltips),m_show_tooltips);
		
	//set speak
	gtk_check_button_set_active (GTK_CHECK_BUTTON(check_button_speak), m_speak);
	gtk_check_button_set_active (GTK_CHECK_BUTTON(check_button_speak_startup), m_speak_at_startup);
	gtk_check_button_set_active (GTK_CHECK_BUTTON(check_button_speak_upcoming), m_speak_upcoming);
	gtk_check_button_set_active (GTK_CHECK_BUTTON(check_button_speak_event_number), m_speak_event_number);
	gtk_check_button_set_active (GTK_CHECK_BUTTON(check_button_speak_time), m_speak_time);
	gtk_check_button_set_active (GTK_CHECK_BUTTON(check_button_speak_event_words), m_speak_event_words);	
		
	gtk_check_button_set_active (GTK_CHECK_BUTTON(check_button_reset_all), m_reset_preferences);

	//data setters
	
	g_object_set_data(G_OBJECT(button_set), "dialog-key",dialog);
	//calendar
	g_object_set_data(G_OBJECT(button_set), "check-button-hour-format-key",check_button_hour_format);
	g_object_set_data(G_OBJECT(button_set), "check-button-show-end-time-key",check_button_show_end_time);
	g_object_set_data(G_OBJECT(button_set), "check-button-holidays-key",check_button_holidays);	
	g_object_set_data(G_OBJECT(button_set), "check-button-show-tooltips-key",check_button_show_tooltips);		
	//speaking
	g_object_set_data(G_OBJECT(button_set), "check-button-speak-key",check_button_speak);
	g_object_set_data(G_OBJECT(button_set), "check-button-speak-startup-key",check_button_speak_startup);
	g_object_set_data(G_OBJECT(button_set), "check-button-speak-upcoming-key",check_button_speak_upcoming);
	g_object_set_data(G_OBJECT(button_set), "check-button-speak-event-number-key",check_button_speak_event_number);
	g_object_set_data(G_OBJECT(button_set), "check-button-speak-time-key",check_button_speak_time);
	g_object_set_data(G_OBJECT(button_set), "check-button-speak-event-words-key",check_button_speak_event_words);
	g_object_set_data(G_OBJECT(button_set), "spin-speak-rate-key", spin_button_speak_rate);
		
	g_object_set_data(G_OBJECT(button_set), "spin-upcoming-days-key", spin_button_upcoming_days);
	
	g_object_set_data(G_OBJECT(button_set), "colour-button-today-key", colour_button_today);
	g_object_set_data(G_OBJECT(button_set), "colour-button-event-key", colour_button_event);	
	g_object_set_data(G_OBJECT(button_set), "colour-button-holiday-key", colour_button_holiday);

	g_object_set_data(G_OBJECT(button_set), "check-button-reset-all-key",check_button_reset_all);
	
	//grid layout
	
	//Calendar preferences	
	gtk_grid_attach(GTK_GRID(grid), check_button_hour_format,   1, 1, 1, 1);			
	gtk_grid_attach(GTK_GRID(grid), check_button_show_end_time, 2, 1, 1, 1);
	
	gtk_grid_attach(GTK_GRID(grid), check_button_holidays,       1, 2, 1, 1);
	gtk_grid_attach(GTK_GRID(grid), check_button_show_tooltips,  2, 2, 1, 1);
	
	gtk_grid_attach(GTK_GRID(grid), label_spacer1,       1, 3, 1, 1);
	
	gtk_grid_attach(GTK_GRID(grid), label_todaycolour,         1, 4, 1, 1);
	gtk_grid_attach(GTK_GRID(grid), colour_button_today,       2, 4, 1, 1);
	
	gtk_grid_attach(GTK_GRID(grid), label_eventcolour,         1, 5, 1, 1);
	gtk_grid_attach(GTK_GRID(grid), colour_button_event,       2, 5, 1, 1);
	
	gtk_grid_attach(GTK_GRID(grid), label_holidaycolour,        1, 6, 1, 1);
	gtk_grid_attach(GTK_GRID(grid), colour_button_holiday,       2, 6, 1, 1);
	
	gtk_grid_attach(GTK_GRID(grid), label_spacer2,       1, 7, 1, 1);
	
	//speak preferences
	gtk_grid_attach(GTK_GRID(grid), check_button_speak,      1, 8, 1, 1);
	gtk_grid_attach(GTK_GRID(grid), label_speak_rate,        2, 8, 1, 1);	
	gtk_grid_attach(GTK_GRID(grid), spin_button_speak_rate,  3, 8, 1, 1);	
			
	gtk_grid_attach(GTK_GRID(grid), check_button_speak_startup,       1, 9, 1, 1);		
	gtk_grid_attach(GTK_GRID(grid), check_button_speak_event_number,  2, 9, 1, 1);	
	gtk_grid_attach(GTK_GRID(grid), check_button_speak_time,          3, 9, 1, 1);		
	
	gtk_grid_attach(GTK_GRID(grid), check_button_speak_event_words,   1, 10, 1, 1);	
	
	gtk_grid_attach(GTK_GRID(grid), check_button_speak_upcoming,      1, 11, 1, 1);	
	gtk_grid_attach(GTK_GRID(grid), label_upcoming_days,             2, 11, 1, 1);		
	gtk_grid_attach(GTK_GRID(grid), spin_button_upcoming_days,       3, 11, 1, 1);
		
	gtk_grid_attach(GTK_GRID(grid), label_spacer3,       1, 12, 1, 1);
	
	gtk_grid_attach(GTK_GRID(grid), check_button_reset_all,  1, 13, 1, 1);
	
	gtk_grid_attach(GTK_GRID(grid), label_spacer4,       1, 14, 1, 1);
	
	gtk_grid_attach(GTK_GRID(grid), button_set,  1, 15, 3, 1);
	
    gtk_window_set_child (GTK_WINDOW (dialog), grid);	
	gtk_window_present(GTK_WINDOW(dialog));
		
}
//=====================================================================

static void callbk_replay_speaking(GSimpleAction *action, GVariant *parameter,  gpointer user_data)
{
		
	GtkWidget *window = user_data;
	
	if(m_speak==0) return;
	if (m_speaking ==TRUE) return;
	
	speak_events();	
	
}

//======================================================================

static void callbk_info(GSimpleAction *action, GVariant *parameter,  gpointer user_data)
{	
	GtkWidget *window =user_data;
	GtkWidget *calendar =g_object_get_data(G_OBJECT(window), "window-calendar-key");
	GtkWidget *dialog;
	GtkWidget *box;
	gint response;
	
	GtkWidget *label_keyboard_shortcuts;	
	GtkWidget *label_home_shortcut;
	GtkWidget *label_newevent_shortcut;
	GtkWidget *label_preferences_shortcut;
	GtkWidget *label_info_shortcut;
	GtkWidget *label_speak_shortcut;
	GtkWidget *label_time_shortcut;
	GtkWidget *label_quit_shortcut;
		
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

	dialog =gtk_window_new();
	
	gtk_window_set_default_size(GTK_WINDOW(dialog),380,100);
	gtk_window_set_title (GTK_WINDOW (dialog), "Information");
	
	box =gtk_box_new(GTK_ORIENTATION_VERTICAL,1);
	gtk_window_set_child (GTK_WINDOW (dialog), box);
	
	label_keyboard_shortcuts=gtk_label_new("Keyboard Shortcuts");
	gtk_label_set_attributes (GTK_LABEL (label_keyboard_shortcuts), attrs);
		
	label_home_shortcut=gtk_label_new("Home: Go to today");	
	label_newevent_shortcut=gtk_label_new("Ctrl+n: New event");
	
	label_preferences_shortcut=gtk_label_new("Ctrl+Alt+P: Preferences");
	label_info_shortcut=gtk_label_new("F1: Information");
	label_speak_shortcut=gtk_label_new("s: Speak selected day events");
	label_time_shortcut=gtk_label_new("t: Speak time");
	label_quit_shortcut=gtk_label_new("Ctrl+q: Quit");

		
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
	gtk_box_append(GTK_BOX(box),label_preferences_shortcut);
	gtk_box_append(GTK_BOX(box),label_info_shortcut);
	gtk_box_append(GTK_BOX(box), label_speak_shortcut);	
	gtk_box_append(GTK_BOX(box), label_time_shortcut);
	gtk_box_append(GTK_BOX(box), label_quit_shortcut);	
		
	gtk_box_append(GTK_BOX(box), label_record_info);
	gtk_box_append(GTK_BOX(box), label_record_number);
	gtk_box_append(GTK_BOX(box), label_sqlite_version);	
		
	gtk_box_append(GTK_BOX(box),label_font_info);
	gtk_box_append(GTK_BOX(box),label_desktop_font);
	gtk_box_append(GTK_BOX(box),label_gnome_text_scale);
		
	pango_attr_list_unref(attrs);
	
	gtk_window_present (GTK_WINDOW (dialog));
	
	gtk_window_set_focus(GTK_WINDOW(window), GTK_WIDGET(calendar));
	
}
//=====================================================================

static void callbk_quit(GSimpleAction * action,	G_GNUC_UNUSED GVariant *parameter, gpointer user_data)
{
	config_write();
	g_application_quit(G_APPLICATION(user_data));		
}


//======================================================================
static void startup(GtkApplication *app)
{	 	
	 config_initialize();	  	
	 db_create_events_table(); //startup database 
}
//======================================================================
void callbk_shutdown(GtkWindow *window, gint response_id, gpointer user_data)
{
	config_write();			
}


//======================================================================
static void window_header(GtkWindow *window)
{
	//header
	GtkWidget *header;
	GtkWidget *button_header;	
	GtkWidget *menu_button;
	GtkWidget *label;
	GtkWidget *button_new_event;
	GtkWidget *button_replay;	
	GtkWidget *popover;
	
	header = gtk_header_bar_new ();  
	gtk_window_set_titlebar (GTK_WINDOW (window), header);
	
	//create headerbar buttons
	button_new_event = gtk_button_new_with_label ("New Event");
	g_signal_connect (button_new_event, "clicked", G_CALLBACK (callbk_new_event), window);
	gtk_header_bar_pack_start(GTK_HEADER_BAR (header), button_new_event);
	
	button_replay = gtk_button_new_from_icon_name ("view-refresh-symbolic");
	gtk_widget_set_tooltip_text(button_replay, "Replay");
	g_signal_connect (button_replay, "clicked", G_CALLBACK (callbk_replay_speaking), window);
  
	//create menu (model)
	GMenu *menu;
	GMenuItem *item;
	menu =g_menu_new();
	
	//File items	
	item =g_menu_item_new("New Event", "app.newevent");
	g_menu_append_item(menu,item);
	g_object_unref(item);		
	
	item =g_menu_item_new("Import", "app.import");
	g_menu_append_item(menu,item);
	g_object_unref(item);
	
	item =g_menu_item_new("Export", "app.export");
	g_menu_append_item(menu,item);
	g_object_unref(item);
	
	item =g_menu_item_new("Preferences", "app.preferences");
	g_menu_append_item(menu,item);
	g_object_unref(item);	
	
	//Search
	item =g_menu_item_new("Search For Events", "app.search");
	g_menu_append_item(menu,item);
	g_object_unref(item);
		
	item =g_menu_item_new("Delete All Events", "app.deleteall");
	g_menu_append_item(menu,item);
	g_object_unref(item);	
		
	//Calendar items
	item =g_menu_item_new("Go To Today", "app.home");
	g_menu_append_item(menu,item);
	g_object_unref(item);	
	
	item =g_menu_item_new("Speak Time", "app.speaktime");
	g_menu_append_item(menu,item);
	g_object_unref(item);
	
	item =g_menu_item_new("Speak Selected Day", "app.speak");
	g_menu_append_item(menu,item);
	g_object_unref(item);
	
	item =g_menu_item_new("Information", "app.info");
	g_menu_append_item(menu,item);
	g_object_unref(item);	
	
	item =g_menu_item_new("About", "app.about");
	g_menu_append_item(menu,item);
	g_object_unref(item);
	
	item =g_menu_item_new("Quit", "app.quit");
	g_menu_append_item(menu,item);
	g_object_unref(item);
		
	//symbolic menu
	//menu_button = gtk_menu_button_new();
	//gtk_widget_set_tooltip_text(menu_button, "Menu");
	//gtk_menu_button_set_icon_name (GTK_MENU_BUTTON (menu_button),"open-menu-symbolic"); 		
	//gtk_menu_button_set_menu_model (GTK_MENU_BUTTON (menu_button), G_MENU_MODEL(menu));
	
	//popver menu
	//Use GTK4 popover i.e. a bubble-like context popup now used to replace menus
	menu_button = gtk_menu_button_new ();	
	popover = gtk_popover_menu_new_from_model(G_MENU_MODEL(menu));
	gtk_menu_button_set_popover(GTK_MENU_BUTTON (menu_button),popover);
	gtk_popover_set_has_arrow(GTK_POPOVER(popover),TRUE);
	
		
	gtk_header_bar_pack_end(GTK_HEADER_BAR (header), button_replay);
	gtk_header_bar_pack_end(GTK_HEADER_BAR (header), menu_button);
	  
}


//======================================================================

static void activate (GtkApplication *app, gpointer  user_data)
{
	
	GtkWidget *window;
	GtkWidget *header;
	GtkWidget *calendar; 
	
	// create a new window, and set its title
	window = gtk_application_window_new (app);
	gtk_window_set_title (GTK_WINDOW (window), "Speaking Calendar "); 		
	g_signal_connect (window, "destroy", G_CALLBACK (callbk_shutdown),app);
	window_header(GTK_WINDOW(window));
			
	//Keyboard accelerators
	const gchar *home_accels[2] = { "Home", NULL };
	const gchar *speak_accels[2] = { "s", NULL };
	const gchar *newevent_accels[2] = {"<Ctrl>n", NULL };	
	const gchar *time_accels[2] = {"t", NULL };
	const gchar *info_accels[2] = {"F1", NULL };	
	const gchar * preferences_accels[2] = { "<Ctrl><Alt>P", NULL };
	const gchar * quit_accels[2] = { "<Ctrl>Q", NULL };
		
	calendar = custom_calendar_new();
	//custom_calendar_goto_today(CUSTOM_CALENDAR(calendar));
	m_start_day = custom_calendar_get_day(CUSTOM_CALENDAR(calendar));
	m_start_month = custom_calendar_get_month(CUSTOM_CALENDAR(calendar));
	m_start_year = custom_calendar_get_year(CUSTOM_CALENDAR(calendar));
		
	m_today_day = custom_calendar_get_day(CUSTOM_CALENDAR(calendar));
	m_today_month = custom_calendar_get_month(CUSTOM_CALENDAR(calendar));
	m_today_year = custom_calendar_get_year(CUSTOM_CALENDAR(calendar));	
		
	g_signal_connect(CUSTOM_CALENDAR(calendar), "day-selected", G_CALLBACK(callbk_calendar_day_selected), window);
	g_signal_connect(CUSTOM_CALENDAR(calendar), "next-month", G_CALLBACK(callbk_calendar_next_month), NULL);
	g_signal_connect(CUSTOM_CALENDAR(calendar), "prev-month", G_CALLBACK(callbk_calendar_prev_month), NULL);
	g_signal_connect(CUSTOM_CALENDAR(calendar), "next-year", G_CALLBACK(callbk_calendar_next_year), NULL);
	g_signal_connect(CUSTOM_CALENDAR(calendar), "prev-year", G_CALLBACK(callbk_calendar_prev_year), NULL);
	
	g_object_set(calendar, "todaycolour", m_todaycolour, NULL);
	g_object_set(calendar, "eventcolour", m_eventcolour, NULL);
	g_object_set(calendar, "holidaycolour", m_holidaycolour, NULL);	
	       
    set_titles_on_calendar(CUSTOM_CALENDAR(calendar));
	custom_calendar_update(CUSTOM_CALENDAR(calendar));
    
    //selection
	m_index=-1;
    m_id_selection=-1;
    
    //setup key-value pairs
	g_object_set_data(G_OBJECT(window), "window-calendar-key",calendar);
	
	
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
	g_signal_connect(deleteevent_action, "activate",  G_CALLBACK(callbk_delete_event), window);
	
	GSimpleAction *deleteall_action;
	deleteall_action=g_simple_action_new("deleteall",NULL); //app.deleteall
	g_action_map_add_action(G_ACTION_MAP(app), G_ACTION(deleteall_action)); //make visible
	g_signal_connect(deleteall_action, "activate",  G_CALLBACK(callbk_delete_all), window);
	
	GSimpleAction *search_action;
	search_action=g_simple_action_new("search",NULL); //app.search
	g_action_map_add_action(G_ACTION_MAP(app), G_ACTION(search_action)); //make visible
	g_signal_connect(search_action, "activate",  G_CALLBACK(callbk_search), window);
	
	//speak actions
	GSimpleAction *speak_action;	
	speak_action=g_simple_action_new("speak",NULL); //app.speak
	g_action_map_add_action(G_ACTION_MAP(app), G_ACTION(speak_action)); //make visible	
	g_signal_connect(speak_action, "activate",  G_CALLBACK(callbk_speak), window);
	
	GSimpleAction *speaktime_action;	
	speaktime_action=g_simple_action_new("speaktime",NULL); //app.speaktime
	g_action_map_add_action(G_ACTION_MAP(app), G_ACTION(speaktime_action)); //make visible	
	g_signal_connect(speaktime_action, "activate",  G_CALLBACK(callbk_speaktime), window);
	
	//Calendar Actions	
	GSimpleAction *home_action;	
	home_action=g_simple_action_new("home",NULL); //app.home
	g_action_map_add_action(G_ACTION_MAP(app), G_ACTION(home_action)); //make visible	
	g_signal_connect(home_action, "activate",  G_CALLBACK(callbk_calendar_home), window);
			
	//Help Actions
	GSimpleAction *info_action;
	info_action=g_simple_action_new("info",NULL); //app.info
	g_action_map_add_action(G_ACTION_MAP(app), G_ACTION(info_action)); //make visible
	g_signal_connect(info_action, "activate",  G_CALLBACK(callbk_info), window);
	
	GSimpleAction *about_action;
	about_action=g_simple_action_new("about",NULL); //app.about
	g_action_map_add_action(G_ACTION_MAP(app), G_ACTION(about_action)); //make visible
	g_signal_connect(about_action, "activate",  G_CALLBACK(callbk_about), window);
		
	// connect keyboard accelerators	
	gtk_application_set_accels_for_action(GTK_APPLICATION(app),"app.home", home_accels);
	gtk_application_set_accels_for_action(GTK_APPLICATION(app),"app.newevent", newevent_accels);
	gtk_application_set_accels_for_action(GTK_APPLICATION(app),"app.speak", speak_accels);		
	gtk_application_set_accels_for_action(GTK_APPLICATION(app),"app.speaktime", time_accels);	
	gtk_application_set_accels_for_action(GTK_APPLICATION(app),"app.info", info_accels);
	gtk_application_set_accels_for_action(GTK_APPLICATION(app),"app.preferences", preferences_accels);
	gtk_application_set_accels_for_action(GTK_APPLICATION(app),"app.quit", quit_accels);
	
	g_timeout_add_seconds(1, callbk_timer_update, NULL);
		
	if(m_speak && m_speak_at_startup) {
		speak_events();		
	}
		
	gtk_window_maximize(GTK_WINDOW(window));
	
	gtk_window_set_child(GTK_WINDOW(window),GTK_WIDGET(calendar));
	
	gtk_window_set_focus(GTK_WINDOW(window), GTK_WIDGET(calendar));
    
	gtk_window_present (GTK_WINDOW (window));    //use present not show with gtk4	
	
}
//======================================================================
int main (int  argc, char **argv)
{
	GtkApplication *app;
	int status;

	app = gtk_application_new ("org.gtk.speakingcalendar", G_APPLICATION_DEFAULT_FLAGS);

	g_signal_connect_swapped(app, "startup", G_CALLBACK (startup),app);

	g_signal_connect (app, "activate", G_CALLBACK (activate), NULL);

	status = g_application_run (G_APPLICATION (app), argc, argv);
	g_object_unref (app);

	return status;
}
