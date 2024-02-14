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

/*
GTK4 Simple Calendar which uses a grid of day labels for each month
Removed all code which used gtk_widget_get_style_context()
gtk_style_context_add_provider() and gtk_color_chooser_get_rgba()
which are being depreciated in gtk4.10. Now uses Pango attributes and 
markup for adding some style and colour to the calendar. Pango is the 
text layout system used by GDK and GTK.
*/

#include "customcalendar.h"

// locals (day and month names do not change)
static char *dayname[7];
static char *monthname[12];

// declarations
static void custom_calendar_button_press(GtkGestureClick *gesture, int n_press, double x, double y, gpointer user_data);

// signals
enum
{
	DAY_SELECTED_SIGNAL,
	PREV_MONTH_SIGNAL,
	NEXT_MONTH_SIGNAL,
	PREV_YEAR_SIGNAL,
	NEXT_YEAR_SIGNAL,
	LAST_SIGNAL
};

static guint custom_calendar_signals[LAST_SIGNAL] = {0};

typedef struct _CustomCalendarClass CustomCalendarClass;
typedef struct _CustomCalendarPrivate CustomCalendarPrivate;

struct _CustomCalendar
{
	// GtkWidget parent_instance;
	GtkWidget widget;
	GtkWidget *btn_next_month;
	GtkWidget *btn_prev_month;
	GtkWidget *btn_next_year;
	GtkWidget *btn_prev_year;
	GtkWidget *grid;
	GtkWidget *day_name_labels[7];
	GtkWidget *day_number_labels[6][7];
	GDateTime *date;
	GtkWidget *month_year_label;
	int day;
	int month;
	int year;
	int day_month[6][7];
	int days[6][7];
	int num_marked_days;
	int marked_day[32];
	int num_marked_holidays;
	int marked_holiday[32];
	const gchar* today_colour;
	const gchar* event_colour;
	const gchar* holiday_colour;
	int scale;
};

struct _CustomCalendarClass
{
	GtkWidgetClass parent_class;
	void (*day_selected)(GtkCalendar *calendar);
	void (*prev_month)(GtkCalendar *calendar);
	void (*next_month)(GtkCalendar *calendar);
	void (*prev_year)(GtkCalendar *calendar);
	void (*next_year)(GtkCalendar *calendar);
};

G_DEFINE_TYPE(CustomCalendar, custom_calendar, GTK_TYPE_WIDGET)

//define properties

//define properties
enum {
    PROP_0,
    PROP_TODAYCOLOUR, 
    PROP_EVENTCOLOUR, 
    PROP_HOLIDAYCOLOUR,   
    LAST_PROP
};

static GParamSpec *properties[LAST_PROP];


static void custom_calendar_set_property(GObject *object,
                                        guint   prop_id,
                                        const GValue  *value,
                                        GParamSpec *pspec)
{
    CustomCalendar *self =(CustomCalendar *)object;    

    switch (prop_id)
    {
        case PROP_TODAYCOLOUR:
            custom_calendar_set_today_colour(self, g_value_get_string(value));
            break;
        case PROP_EVENTCOLOUR:
            custom_calendar_set_event_colour(self, g_value_get_string(value));
            break;
        case PROP_HOLIDAYCOLOUR:
            custom_calendar_set_holiday_colour(self, g_value_get_string(value));
            break;
	}
}

static void custom_calendar_get_property(GObject *object,
                           guint         prop_id,
                           GValue       *value,
                           GParamSpec   *pspec)
{
  CustomCalendar *self =(CustomCalendar *)object;

  switch (prop_id)
    {
    case PROP_TODAYCOLOUR:
      g_value_set_string(value, custom_calendar_get_today_colour(self));
      break;
    case PROP_EVENTCOLOUR:
      g_value_set_string(value, custom_calendar_get_event_colour(self));
      break;
    case PROP_HOLIDAYCOLOUR:
      g_value_set_string(value, custom_calendar_get_holiday_colour(self));
      break;
  }
}

static void custom_calendar_dispose(GObject *object)
{
	CustomCalendar *calendar = CUSTOM_CALENDAR(object);
	g_clear_pointer(&calendar->grid, gtk_widget_unparent);
	G_OBJECT_CLASS(custom_calendar_parent_class)->dispose(object);
}

static void custom_calendar_class_init(CustomCalendarClass *klass)
{
	 GObjectClass *object_class =G_OBJECT_CLASS(klass);
	
	GtkWidgetClass *widget_class;
	widget_class = (GtkWidgetClass *)klass;

	object_class->dispose = custom_calendar_dispose;
	
	//properies
	
	//override these
	object_class->set_property = custom_calendar_set_property;
	object_class->get_property = custom_calendar_get_property;
	
	//GParamSpec* g_param_spec_string (
		  //const gchar* name,
		  //const gchar* nick,
		  //const gchar* blurb,
		  //const gchar* default_value,
		  //GParamFlags flags
		//)
	
	
	//properties
    properties[PROP_TODAYCOLOUR] =
    g_param_spec_string("todaycolour",
                        "todaycolour",
                        "HTML colour name for today",
                        "red",
                        (G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));
                        
   
                        
    properties[PROP_EVENTCOLOUR] =
    g_param_spec_string("eventcolour",
                        "eventcolour",
                        "HTML colour name for an event",
                        "brown",
                        (G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));
     
    properties[PROP_HOLIDAYCOLOUR] =
    g_param_spec_string("holidaycolour",
                        "holidaycolour",
                        "HTML colour name for a holiday",
                        "blue",
                        (G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));
     
     
     g_object_class_install_properties(object_class, LAST_PROP, properties);                  
	
	
	//Signals emitted on user actions
	
	custom_calendar_signals[DAY_SELECTED_SIGNAL] =
		g_signal_new("day-selected",
					 G_OBJECT_CLASS_TYPE(object_class),
					 G_SIGNAL_RUN_FIRST,
					 G_STRUCT_OFFSET(CustomCalendarClass, day_selected),
					 NULL, NULL,
					 NULL,
					 G_TYPE_NONE, 0);

	custom_calendar_signals[NEXT_MONTH_SIGNAL] =
		g_signal_new("next-month",
					 G_OBJECT_CLASS_TYPE(object_class),
					 G_SIGNAL_RUN_FIRST,
					 G_STRUCT_OFFSET(CustomCalendarClass, next_month),
					 NULL, NULL,
					 NULL,
					 G_TYPE_NONE, 0);

	custom_calendar_signals[PREV_MONTH_SIGNAL] =
		g_signal_new("prev-month",
					 G_OBJECT_CLASS_TYPE(object_class),
					 G_SIGNAL_RUN_FIRST,
					 G_STRUCT_OFFSET(CustomCalendarClass, prev_month),
					 NULL, NULL,
					 NULL,
					 G_TYPE_NONE, 0);

	custom_calendar_signals[NEXT_YEAR_SIGNAL] =
		g_signal_new("next-year",
					 G_OBJECT_CLASS_TYPE(object_class),
					 G_SIGNAL_RUN_FIRST,
					 G_STRUCT_OFFSET(CustomCalendarClass, next_year),
					 NULL, NULL,
					 NULL,
					 G_TYPE_NONE, 0);

	custom_calendar_signals[PREV_YEAR_SIGNAL] =
		g_signal_new("prev-year",
					 G_OBJECT_CLASS_TYPE(object_class),
					 G_SIGNAL_RUN_FIRST,
					 G_STRUCT_OFFSET(CustomCalendarClass, prev_year),
					 NULL, NULL,
					 NULL,
					 G_TYPE_NONE, 0);

	gtk_widget_class_set_layout_manager_type(widget_class, GTK_TYPE_BOX_LAYOUT);
}


GtkWidget *custom_calendar_new(void)
{	
	return g_object_new(CUSTOM_TYPE_CALENDAR, NULL);
}

//---------------------------------------------------------------------

void custom_calendar_reset_marks(CustomCalendar *calendar)
{
	g_return_if_fail(CUSTOM_IS_CALENDAR(calendar));

	// day 0 not used but set to zero
	//  array size 32 array index [0,1,.....31]

	for (int i = 0; i < 32; i++){
		calendar->marked_day[i] = FALSE;		
	}

	calendar->num_marked_days = 0;	
}

void custom_calendar_reset_holidays(CustomCalendar *calendar)
{
	g_return_if_fail(CUSTOM_IS_CALENDAR(calendar));

	// day 0 not used but set to zero
	//  array size 32 array index [0,1,.....31]

	for (int i = 0; i < 32; i++){		
		calendar->marked_holiday[i] = FALSE;
	}
	
	calendar->num_marked_holidays = 0;
}

void custom_calendar_mark_day(CustomCalendar *calendar, guint day)
{
	g_return_if_fail(CUSTOM_IS_CALENDAR(calendar));
		
	if (day >= 1 && day <= 31)
	{
		calendar->marked_day[day] = TRUE;
		calendar->num_marked_days++;
	}	
}

void custom_calendar_unmark_day(CustomCalendar *calendar, guint day)
{
	g_return_if_fail(CUSTOM_IS_CALENDAR(calendar));	

	if (day >= 1 && day <= 31)
	{
		calendar->marked_day[day] = FALSE;
		calendar->num_marked_days--;
	}
}

void custom_calendar_mark_holiday(CustomCalendar *calendar, guint day)
{
	g_return_if_fail(CUSTOM_IS_CALENDAR(calendar));
	
	if (day >= 1 && day <= 31)
	{
		calendar->marked_holiday[day] = TRUE;
		calendar->num_marked_holidays++;
	}	
}

void custom_calendar_unmark_holiday(CustomCalendar *calendar, guint day)
{
	g_return_if_fail(CUSTOM_IS_CALENDAR(calendar));	

	if (day >= 1 && day <= 31)
	{
		calendar->marked_holiday[day] = FALSE;
		calendar->num_marked_holidays--;
	}
}


gboolean custom_calendar_get_day_is_marked(CustomCalendar *calendar, guint day)
{
	if (day >= 1 && day <= 31)
		return calendar->marked_day[day];

	return FALSE;
}

static int setup_monthname()
{
	int i;
	for (i = 0; i < 12; i++)
	{	
		switch (i)
		{
		case 0:
			monthname[i] = g_strdup("January");
			break;
		case 1:
			monthname[i] = g_strdup("Feburary");
			break;
		case 2:
			monthname[i] = g_strdup("March");
			break;
		case 3:
			monthname[i] = g_strdup("April");
			break;
		case 4:
			monthname[i] = g_strdup("May");
			break;
		case 5:
			monthname[i] = g_strdup("June");
			break;
		case 6:
			monthname[i] = g_strdup("July");
			break;
		case 7:
			monthname[i] = g_strdup("August");
			break;
		case 8:
			monthname[i] = g_strdup("September");
			break;
		case 9:
			monthname[i] = g_strdup("October");
			break;
		case 10:
			monthname[i] = g_strdup("November");
			break;
		case 11:
			monthname[i] = g_strdup("December");
			break;
		default:
			monthname[i] = g_strdup("Month");
		}
	}
}

static int setup_dayname()
{
	int i;
	for (i = 0; i < 7; i++)
	{
		switch (i)
		{
		case 0:
			dayname[i] = g_strdup("Mon");
			break;
		case 1:
			dayname[i] = g_strdup("Tue");
			break;
		case 2:
			dayname[i] = g_strdup("Wed");
			break;
		case 3:
			dayname[i] = g_strdup("Thu");
			break;
		case 4:
			dayname[i] = g_strdup("Fri");
			break;
		case 5:
			dayname[i] = g_strdup("Sat");
			break;
		case 6:
			dayname[i] = g_strdup("Sun");
			break;
		default:
			dayname[i] = g_strdup("Day");
		}
	}
}

static int first_day_of_month(int month, int year)
{
	if (month < 3)
	{
		month += 12;
		year--;
	}
	int century = year / 100;
	year = year % 100;
	return (((13 * (month + 1)) / 5) +
			(century / 4) + (5 * century) +
			year + (year / 4)) %
		   7;
}

static void custom_calendar_init(CustomCalendar *calendar)
{

	GtkWidget *widget = GTK_WIDGET(calendar);
	
	calendar->day = 0;
	calendar->month = 0;
	calendar->year = 0;
		
	gtk_widget_set_focusable(widget, TRUE);	
	setup_dayname();
	setup_monthname();
	custom_calendar_reset_marks(calendar);

	int week_start = 0; // local

	int n_cols = 7;
	int n_rows = 8;
	GtkWidget *button;
	GtkGesture *gesture;

	gesture = gtk_gesture_click_new();
	g_signal_connect(gesture, "pressed", G_CALLBACK(custom_calendar_button_press), calendar);
	gtk_widget_add_controller(GTK_WIDGET(calendar), GTK_EVENT_CONTROLLER(gesture));

	calendar->btn_next_month = gtk_button_new_from_icon_name("pan-end-symbolic");	
	gtk_widget_set_tooltip_text(calendar->btn_next_month, "Next Month");
	g_signal_connect_swapped(calendar->btn_next_month, "clicked", G_CALLBACK(callbk_next_month), calendar);

	calendar->btn_next_year = gtk_button_new_from_icon_name("pan-end-symbolic");
	gtk_widget_set_tooltip_text(calendar->btn_next_year, "Next Year");
	g_signal_connect_swapped(calendar->btn_next_year, "clicked", G_CALLBACK(callbk_next_year), calendar);

	calendar->btn_prev_month = gtk_button_new_from_icon_name("pan-start-symbolic");
	gtk_widget_set_tooltip_text(calendar->btn_prev_month, "Previous Month");
	g_signal_connect_swapped(calendar->btn_prev_month, "clicked", G_CALLBACK(callbk_prev_month), calendar);

	calendar->btn_prev_year = gtk_button_new_from_icon_name("pan-start-symbolic");
	gtk_widget_set_tooltip_text(calendar->btn_prev_year, "Previous Year");
	g_signal_connect_swapped(calendar->btn_prev_year, "clicked", G_CALLBACK(callbk_prev_year), calendar);

	GtkWidget *label_my = gtk_label_new("month-year");
	gtk_widget_set_hexpand(label_my, TRUE);
	gtk_widget_set_vexpand(label_my, TRUE);
	calendar->month_year_label = label_my;

	calendar->grid = gtk_grid_new();
	gtk_grid_set_row_homogeneous(GTK_GRID(calendar->grid), TRUE);
	gtk_grid_set_column_homogeneous(GTK_GRID(calendar->grid), TRUE);

	for (int i = 0; i < 7; i++)
	{
		int day;
		GtkWidget *label;

		day = (i + week_start) % 7;
		label = gtk_label_new(dayname[day]);

		gtk_widget_set_hexpand(label, TRUE);
		gtk_widget_set_vexpand(label, TRUE);			
		
		PangoAttrList *attr;
		attr = pango_attr_list_new();
		pango_attr_list_insert(attr, pango_attr_weight_new(PANGO_WEIGHT_BOLD));
		pango_attr_list_insert(attr, pango_attr_underline_new(PANGO_UNDERLINE_SINGLE));	
		gtk_label_set_attributes(GTK_LABEL(label), attr);
		pango_attr_list_unref(attr);
		
		gtk_grid_attach(GTK_GRID(calendar->grid), label, i, 1, 1, 1);

		calendar->day_name_labels[i] = label;
	}

	// Grid is 7 days

	/*
	 - -*------------------------------------
	 0  | 1  | 2  | 3   | 4  | 5  | 6  |
	 Mon Tue  Wed  Thu  Fri  Sat  Sun

	 */

	gtk_grid_attach(GTK_GRID(calendar->grid), calendar->btn_prev_year, 0, 0, 1, 1);
	gtk_grid_attach(GTK_GRID(calendar->grid), calendar->btn_prev_month, 1, 0, 1, 1);
	gtk_grid_attach(GTK_GRID(calendar->grid), calendar->month_year_label, 2, 0, 3, 1);
	gtk_grid_attach(GTK_GRID(calendar->grid), calendar->btn_next_month, 5, 0, 1, 1);
	gtk_grid_attach(GTK_GRID(calendar->grid), calendar->btn_next_year, 6, 0, 1, 1);

	// initialise
	for (int y = 0; y < 6; y++)
	{
		for (int x = 0; x < 7; x++)
		{
			GtkWidget *label = gtk_label_new("");
			
			gtk_label_set_use_markup(GTK_LABEL(label), TRUE);			

			gtk_widget_set_hexpand(label, TRUE);
			gtk_widget_set_vexpand(label, TRUE);			
			
  
			gtk_grid_attach(GTK_GRID(calendar->grid), label, x, y + 2, 1, 1);
			calendar->day_number_labels[y][x] = label; // initialise
		}
	}

	// get local date at startup
	GDateTime *now;
	now = g_date_time_new_now_local();
	calendar->year = g_date_time_get_year(now);
	calendar->month = g_date_time_get_month(now);
	calendar->day = g_date_time_get_day_of_month(now);	
	custom_calendar_select_day(calendar, calendar->day, calendar->month, calendar->year);
	g_date_time_unref(now); // freeit quick before it leaks

	gtk_widget_set_hexpand(calendar->grid, TRUE);
	gtk_widget_set_vexpand(calendar->grid, TRUE);
	gtk_widget_set_parent(calendar->grid, GTK_WIDGET(calendar));
}

int custom_calendar_get_day(CustomCalendar *calendar)
{
	return calendar->day;
}

int custom_calendar_get_month(CustomCalendar *calendar)
{
	return calendar->month;
}
int custom_calendar_get_year(CustomCalendar *calendar)
{
	return calendar->year;
}

static void custom_calendar_select_day(CustomCalendar *calendar, guint dday, guint month, guint year)
{
	
	const gchar* today_colour_str=calendar->today_colour;
	const gchar* event_colour_str=calendar->event_colour;
	const gchar* holiday_colour_str=calendar->holiday_colour;
		
	gchar* today_markup_str="<span foreground=";	
	today_markup_str= g_strconcat(today_markup_str,"\"",today_colour_str,"\""," weight=\"bold\">%s</span>", NULL);	
	//g_print("today_markup_str = %s", today_markup_str);
	
	gchar* event_markup_str="<span foreground=";	
	event_markup_str= g_strconcat(event_markup_str,"\"",event_colour_str,"\""," weight=\"bold\">%s</span>", NULL);	
	//g_print("event_markup_str = %s", event_markup_str);
	
	gchar* holiday_markup_str="<span foreground=";	
	holiday_markup_str= g_strconcat(holiday_markup_str,"\"",holiday_colour_str,"\""," weight=\"bold\">%s</span>", NULL);	
	//g_print("holiday_markup_str = %s", holiday_markup_str);
		
	int aday = 0;
	int first_day_month = first_day_of_month(calendar->month, calendar->year);
	int week_start = 1; // start week on a Monday
	int remainder = (first_day_of_month(calendar->month, calendar->year) - week_start + 7) % 7;
	aday = 1 - remainder; // day with offset
	int n_cols = 7;
	int n_rows = 8;
	bool today_flag=FALSE;

	int days_in_month = g_date_get_days_in_month(calendar->month, calendar->year);
		
	gchar *month_year_str = "";
	gchar *year_str = g_strdup_printf("%d", calendar->year);
	gchar *month_str = monthname[calendar->month - 1];
	month_year_str = g_strconcat(month_year_str, " ", month_str, " ", year_str, NULL);
	
	gtk_label_set_label(GTK_LABEL(calendar->month_year_label), month_year_str);
	gtk_label_set_label(GTK_LABEL(calendar->month_year_label), month_year_str);

	GDate *today_date;
	today_date = g_date_new();
	g_date_set_time_t(today_date, time(NULL));
	int today_day = g_date_get_day(today_date);
	int today_month = g_date_get_month(today_date);
	int today_year = g_date_get_year(today_date);
	g_date_free(today_date); // freeit quick before it leaks

	// reset labels
	for (int y = 0; y < 6; y++)
		for (int x = 0; x < 7; x++)
		{
			gtk_label_set_label(GTK_LABEL(calendar->day_number_labels[y][x]), "");
		}

	// scale week days
	for (int i = 0; i < 7; i++)
	{
		
		PangoAttrList *attr;
		attr = pango_attr_list_new();	
		pango_attr_list_insert(attr, pango_attr_weight_new(PANGO_WEIGHT_BOLD));
		pango_attr_list_insert(attr, pango_attr_underline_new(PANGO_UNDERLINE_SINGLE));
		gtk_label_set_attributes(GTK_LABEL(calendar->day_name_labels[i]), attr);
		pango_attr_list_unref(attr);
	}
	
	gchar *aday_str = "";
	// update day labels
	for (int y = 0; y < 6; y++)
	{
		for (int x = 0; x < 7; x++)
		{
			
			if (aday > 0 && aday <= days_in_month)
			{
				calendar->days[y][x] = aday;
				gchar buffer[512];
				
				// add mark color
				if (calendar->marked_day[aday])
				{
					
					//if today					
					if (aday == today_day && calendar->month == today_month && calendar->year == today_year)
					{
						//today and marked
						gchar *today_str ="";						
						today_str = g_strdup_printf("%s%s", g_strdup_printf("%i", aday), "*");						
						g_snprintf(buffer, sizeof(buffer),today_markup_str, today_str);
						gtk_label_set_label(GTK_LABEL(calendar->day_number_labels[y][x]), buffer);
					
					}
					else
					{
						//normal day marked												
						aday_str = g_strdup_printf("%s%s", g_strdup_printf("%i", aday), "<b>*</b>");
						
						if (calendar->marked_holiday[aday])
						{
							//mark holidays							
							g_snprintf(buffer, sizeof(buffer),holiday_markup_str, aday_str);
							gtk_label_set_label(GTK_LABEL(calendar->day_number_labels[y][x]), buffer);
						}
						else {
													
							g_snprintf(buffer, sizeof(buffer),event_markup_str, aday_str);
							gtk_label_set_label(GTK_LABEL(calendar->day_number_labels[y][x]), buffer);
						}				
						
					}

				
				}
				else
				{
					if (aday == today_day && calendar->month == today_month && calendar->year == today_year)
					{
												
						//today colour
						gchar *today_str ="";						
						today_str = g_strconcat(today_str,g_strdup_printf("%i", aday),NULL);												
						g_snprintf(buffer, sizeof(buffer),today_markup_str, today_str);
						gtk_label_set_label(GTK_LABEL(calendar->day_number_labels[y][x]), buffer);											
					
					}
					else
					{
						//normal days
						if (calendar->marked_holiday[aday])
						{							
							//normal no event day but mark holidays 
							aday_str = g_strdup_printf("%s%s", g_strdup_printf("%i", aday), "");
							g_snprintf(buffer, sizeof(buffer),holiday_markup_str, aday_str);							
							gtk_label_set_label(GTK_LABEL(calendar->day_number_labels[y][x]), buffer);	
							//g_free(holiday_markup_str);					
						}
						else {
							aday_str = g_strdup_printf("%d", aday);
							gtk_label_set_label(GTK_LABEL(calendar->day_number_labels[y][x]), aday_str);
						}						
					}
				}	
			}
			aday = aday + 1;
		}
	}	
}


void custom_calendar_goto_today(CustomCalendar *calendar)
{	
	GDateTime *today;
	today = g_date_time_new_now_local();
	calendar->year = g_date_time_get_year(today);
	calendar->month = g_date_time_get_month(today);
	calendar->day = g_date_time_get_day_of_month(today);
	custom_calendar_select_day(calendar, calendar->day, calendar->month, calendar->year);
	g_date_time_unref(today);
}



static void callbk_next_month(CustomCalendar *calendar)
{	
	calendar->month = calendar->month + 1;
	calendar->day = 1;
	if (calendar->month >= 13)
	{
		calendar->month = 1;
		calendar->year = calendar->year + 1;
		calendar->day = 1;
	}	
	custom_calendar_select_day(calendar, calendar->day, calendar->month, calendar->year);	
	g_signal_emit(calendar, custom_calendar_signals[NEXT_MONTH_SIGNAL], 0);
}

static void callbk_prev_month(CustomCalendar *calendar)
{
	calendar->month = calendar->month - 1;
	calendar->day = 1;
	if (calendar->month < 1)
	{
		calendar->month = 12;
		calendar->year = calendar->year - 1;
		calendar->day = 1;
	}
	custom_calendar_select_day(calendar, calendar->day, calendar->month, calendar->year);	
	g_signal_emit(calendar, custom_calendar_signals[PREV_MONTH_SIGNAL], 0);
}

static void callbk_next_year(CustomCalendar *calendar)
{
	calendar->year = calendar->year + 1;
	calendar->month = calendar->month;
	calendar->day = 1;
	custom_calendar_select_day(calendar, calendar->day, calendar->month, calendar->year);	
	g_signal_emit(calendar, custom_calendar_signals[NEXT_YEAR_SIGNAL], 0);
}

static void callbk_prev_year(CustomCalendar *calendar)
{
	calendar->year = calendar->year - 1;
	calendar->month = calendar->month;
	calendar->day = 1;
	custom_calendar_select_day(calendar, calendar->day, calendar->month, calendar->year);
	g_signal_emit(calendar, custom_calendar_signals[PREV_YEAR_SIGNAL], 0);
}

static void custom_calendar_button_press(GtkGestureClick *gesture, int n_press, double x, double y, gpointer user_data)
{
	CustomCalendar *calendar = user_data;
	GtkWidget *widget = GTK_WIDGET(calendar);
	GtkWidget *label;
	int row = -1, col = -1;
	int ix, iy;
	int day_month;
	int aday;

	label = gtk_widget_pick(widget, x, y, GTK_PICK_DEFAULT);
	for (iy = 0; iy < 6; iy++)
		for (ix = 0; ix < 7; ix++)
		{
			if (label == calendar->day_number_labels[iy][ix])
			{
				row = iy;
				col = ix;
			}
		}
		
	//check for valid days(to do!)
	//critical g_date_time_get_day_of_week: assertion 'datetime != NULL' failed	

	if (row == -1 || col == -1)
		return;

	day_month = calendar->day_month[row][col];
	aday = calendar->days[row][col];
	
	calendar->day = aday;	
	//printf("day = %d\n",aday);

	if (!gtk_widget_has_focus(widget))
		gtk_widget_grab_focus(widget);


	custom_calendar_select_day(calendar, calendar->day, calendar->month, calendar->year);	
	g_signal_emit(calendar, custom_calendar_signals[DAY_SELECTED_SIGNAL], 0);
}

void custom_calendar_update(CustomCalendar *calendar)
{
	g_return_if_fail(CUSTOM_IS_CALENDAR(calendar));
	custom_calendar_select_day(calendar, calendar->day, calendar->month, calendar->year);
}


//---------------------------------------------------------------------
//properties
//----------------------------------------------------------------------
void custom_calendar_set_today_colour(CustomCalendar *self, const gchar* colourname)
{
	self->today_colour =colourname;	
	//g_print("setting today colour = %s\n",self->today_colour);
}
const gchar* custom_calendar_get_today_colour(CustomCalendar *self){
	
	return self->today_colour;
}
//---------------------------------------------------------------

void custom_calendar_set_event_colour(CustomCalendar *self, const gchar* colourname)
{
	self->event_colour =colourname;	
	//g_print("setting event colour = %s\n",self->event_colour);
}

const gchar* custom_calendar_get_event_colour(CustomCalendar *self){
	
	return self->event_colour;
}
//----------------------------------------------------------------------

void custom_calendar_set_holiday_colour(CustomCalendar *self, const gchar* colourname)
{
	self->holiday_colour =colourname;	
	//g_print("setting holiday colour = %s\n",self->holiday_colour);
}

const gchar* custom_calendar_get_holiday_colour(CustomCalendar *self){
	
	return self->holiday_colour;
}
//----------------------------------------------------------------------
