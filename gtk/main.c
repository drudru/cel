
/* example-start helloworld2 helloworld2.c */

#include <gtk/gtk.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <string.h>
#include <stdlib.h>


#include "svm32.h"

#include "celendian.h"
#include "atom.h"
#include "runtime.h"
#include "sd2cel.h"
#include "assemble.h"
#include "array.h"

#include <stdio.h>
#include <sys/syscall.h>
#include <unistd.h>



GtkWidget *MainWindow;
VM32Cpu * CPU;

// A Global...
GtkWidget *termBox;
GtkWidget *textBox;
GtkWidget *clist;
GtkWidget *reglist;
GdkFont  *fixed_font;
GdkColormap *cmap;
GdkColor colorRed;
GdkColor colorGreen;

int Trace = 0;
Atom BreakMesg;

int debugParse;
int debugParseLevel = -1;
int debugCodeGen;
int debugForce;

extern char **environ;
         

void consoleOut( char * text, int color, int length );

void setHumanReadable(char * str, Proto proto)
{
  int i;
  Proto p1;
  char buff[40];

  // always initialize the string

  str[0] = 0;
  i = (uint32) proto & PROTO_REF_TYPE;

  p1 = (struct _proto *)((uint32) proto & PROTO_REF_MASK);

  if (proto == nil) {
	sprintf(str, " <nil> ");
	return;
  }

  switch (i) {
    case PROTO_REF_INT:
      // display the int
      sprintf(str, "%ld", objectIntegerValue(proto) );
      return;
      break;
    case PROTO_REF_ATOM:
      // display the int
      sprintf(str, "\"%s\"", atomToString((Atom)proto) );
      return;
      break;
    case PROTO_REF_PROTO:
      str[0] = 0;
/* PUNT - we had problems - there are bad memory addresses
          on the stack.
      // if it is a forwarder, print the first object and the object
      // pointed to
      if (p1->header & PROTO_FLAG_FORWARDER) {
        sprintf(buff, "[0x%lx]->", ((uint32) proto & PROTO_REF_MASK));
	strcat(str, buff);

        while (1) {
	  p1 = ((Proto)p1->count);
	  if ( (p1->header & PROTO_FLAG_FORWARDER) == 0) {
	    break;
          }
        }
      }
      */

      sprintf(buff, "<0x%lx>", ((uint32) p1 & PROTO_REF_MASK));
      strcat(str, buff);
      return;
      break;
  }

  strcat(str, "??");

}

/* User clicked the "Add List" button. */
void doDebuggerDisplay(void)
{
      char * opcode;
      int len;
      char buff[128];
      int operand;
      int i;
      // Reg, Addr, Value, D
      gchar *stack[4];
      gchar *strNull = "";
      gchar *strFP = "FP";
      gchar *strSP = "SP";
      gchar *strPC = "PC";
      gchar reg[16];
      gchar addr[16];
      gchar value[16];
      gchar humanReadable[40];
      uint32 * p;
 
      /* Show the next instruction to execute */
      /* Given the program counter, disassemble the next instruction
         and display it */
      i = disassembleCpu(CPU, &opcode, &len);
      if (i) {
        if (len) {
          operand = * (int *) ((CPU->pc) + 1);
          sprintf(buff, "0x%0x: %0x  %s  %0x\n", CPU->pc,*(char *) (CPU->pc), opcode, (unsigned int) std2hostInteger(operand)  );
        } else {
          sprintf(buff, "0x%0x: %0x  %s\n", CPU->pc, *(char*)(CPU->pc), opcode);
        }

	consoleOut( buff, 1, -1);
         
      }
	
      /* Write the disassemble module in the vm tree DRUDRU */

      /* Show the stack as it is */
      /* Clear the list, and add new items to the list */
      /* Our stack grows lower in memory */

      gtk_clist_clear( (GtkCList *) reglist);
       stack[0] = strPC;
       sprintf(addr, "0x%08lx", (uint32) CPU->pc);
       stack[1] = addr;
       stack[2] = strNull;
       gtk_clist_append( (GtkCList *) reglist, stack);

       stack[0] = strSP;
       sprintf(addr, "0x%08lx", (uint32) CPU->sp);
       stack[1] = addr;
       stack[2] = strNull;
       gtk_clist_append( (GtkCList *) reglist, stack);

       stack[0] = strFP;
       sprintf(addr, "0x%08lx", (uint32) CPU->fp);
       stack[1] = addr;
       stack[2] = strNull;
       gtk_clist_append( (GtkCList *) reglist, stack);


      gtk_clist_clear( (GtkCList *) clist);
      /* Here we do the actual adding of the text. It's done once for
       * each row.
       */
      p = (uint32 *) CPU->sp;
      p = p + 20;
      for ( i = 0; i < 22; i++ ) {

          reg[0] = 0;
          if ( p == (uint32 *) CPU->fp ) {
            strcat(reg, strFP);
	  } 
          if ( p ==  (uint32 *)(CPU->sp + 4) ) {
            strcat(reg, strSP);
            strcat(reg, " > ");
          }
          stack[0] = reg;

	  sprintf(addr, "0x%08lx", (uint32) p);
	  stack[1] = addr;

	  sprintf(value, "0x%08lx", (uint32) *p);
	  stack[2] = value;

	  setHumanReadable(humanReadable, (Proto) *p);
          stack[3] = humanReadable;
          gtk_clist_append( (GtkCList *) clist, stack);
	
          p--;
      }

      return;
}

/* User clicked the "Add List" button. */
void button_add_clicked( gpointer data )
      {
          int indx;
       
          /* Something silly to add to the list. 6 rows of 4 columns each */
          gchar *drink[6][4] = {
		 { "",    "Milk",      "3 Oz",     "<Blob>"             },
                 { "",    "Water",     "6 l",      "<Int: 6>"           },
                 { "SP>", "Carrots",   "2",        "<Int: 0>"           },
                 { "FP>", "Snakes",    "55",       "<Proto: 0x1234548>" },
                 { "",    "00001234",  "000000A4", "<Proto: 0x1234548>" },
                 { "",    "06781234",  "90034448", "<Proto: 0x8765432>" }
          };

          /* Here we do the actual adding of the text. It's done once for
           * each row.
           */
          for ( indx=0 ; indx < 6 ; indx++ )
              gtk_clist_append( (GtkCList *) data, drink[indx]);

          return;
      }

      /* User clicked the "Clear List" button. */
      void button_clear_clicked( gpointer data )
      {
          /* Clear the list using gtk_clist_clear. This is much faster than
           * calling gtk_clist_remove once for each row.
           */
          gtk_clist_clear( (GtkCList *) data);

          return;
      }

      /* The user clicked the "Hide/Show titles" button. */
      void button_hide_show_clicked( gpointer data )
      {
          /* Just a flag to remember the status. 0 = currently visible */
          static short int flag = 0;

          if (flag == 0)
          {
              /* Hide the titles and set the flag to 1 */
              gtk_clist_column_titles_hide((GtkCList *) data);
              flag++;
          }
          else
          {
              /* Show the titles and reset flag to 0 */
              gtk_clist_column_titles_show((GtkCList *) data);
              flag--;
          }

          return;
      }

      /* If we come here, then the user has selected a row in the list. */
      void selection_made( GtkWidget      *clist,
                           gint            row,
                           gint            column,
                           GdkEventButton *event,
                           gpointer        data )
      {
          gchar *text;

          /* Get the text that is stored in the selected row and column
           * which was clicked in. We will receive it as a pointer in the
           * argument text.
           */
          gtk_clist_get_text(GTK_CLIST(clist), row, column, &text);

          /* Just prints some information about the selected row */
          g_print("You selected row %d. More specifically you clicked in "
                  "column %d, and the text in this cell is %s\n\n",
                  row, column, text);

          return;
      }


/* Our new improved callback.  The data passed to this function
 * is printed to stdout. */
void callback( GtkWidget *widget, gpointer   data )
      {
          singleStep(CPU);
          doDebuggerDisplay();
      }

void runCallback( GtkWidget *widget, gpointer   data )
      {
    unsigned char inst;
    unsigned char * ptr;
    Atom mesg;
    char buff[16];
    
	  while (1) {

    	    /* Get an instruction */
    	    ptr = (char *) CPU->pc;
    	    inst = *ptr;

	    // If it is a Call
	    if (inst == 0x02) {
               mesg = (Atom) stackAt(Cpu, 2);

               if (mesg == BreakMesg) {
                sprintf(buff, "Mesg Break!\n");
                consoleOut(buff, 0, -1);

                doDebuggerDisplay();
		 break;
	       }
            }

            singleStep(CPU);
	    if (Trace) {
              doDebuggerDisplay();
            }
      }
}

void adjCallback( GtkWidget *widget, gpointer   data )
      {
      	  printf("Value->%0f ", GTK_ADJUSTMENT(widget)->value);
      	  printf("Upper->%0f ", GTK_ADJUSTMENT(widget)->upper);
      	  printf("PageSize->%0f\n", GTK_ADJUSTMENT(widget)->page_size);
      }

void dumpCallback( GtkWidget *widget, gpointer   data )
{
      char buff[128];
      char * p;
      uint32 obj;

      gchar *entry_text;
      entry_text = gtk_entry_get_text(GTK_ENTRY(textBox));
      sprintf(buff, "Object Dump - %s \n", (char *) entry_text);
      consoleOut(buff, 0, -1);

      obj = strtol(entry_text, &p, 16);
      if (*p == 0) {
        objectDump((Proto) obj);
      }
      // Clear the entry
      gtk_entry_set_text( GTK_ENTRY(textBox), "" );
}

void breakCallback( GtkWidget *widget, gpointer   data )
{
      char buff[128];

      gchar *entry_text;
      entry_text = gtk_entry_get_text(GTK_ENTRY(textBox));
      sprintf(buff, "Break Point - %s ...", (char *) entry_text);
      consoleOut(buff, 0, -1);

      BreakMesg = stringToAtom(entry_text);

      sprintf(buff, "Set\n");
      consoleOut(buff, 0, -1);

      // Clear the entry
      gtk_entry_set_text( GTK_ENTRY(textBox), "" );
}

void traceToggle( GtkWidget *widget, gpointer   data )
{
     if (GTK_TOGGLE_BUTTON(widget)->active) {
       Trace = 1;
       g_print ("Toggle active - %s was pressed\n", (char *) data);
     } else {
       Trace = 0;
       g_print ("Toggle in-active - %s was pressed\n", (char *) data);
     }
}

void comm_callback( GtkWidget *widget, gpointer   data )
{
          gchar *entry_text;
          entry_text = gtk_entry_get_text(GTK_ENTRY(data));
          g_print ("Text Entered - %s was pressed\n", (char *) entry_text);

	  consoleOut(entry_text, 0, -1);
	  gtk_entry_set_text( GTK_ENTRY(data), "" );
}


int dbgIoOut(char * str, int count) 
{

  consoleOut(str, 2, count);
  
  return 0;
}

/* colors: 0 = black, 1 = red, 2 = green, 3 = blue */
void consoleOut( char * text, int color, int length)
{
	GtkAdjustment * adj;

	  /* Realizing a widget creates a window for it, ready for us to insert some text */
         gtk_widget_realize (termBox);

         /* Freeze the text widget, ready for multiple updates */
         gtk_text_freeze (GTK_TEXT (termBox));
         
         /* Insert some coloured text */
         if (0 == color) {
           gtk_text_insert (GTK_TEXT (termBox), fixed_font, &termBox->style->black, NULL, text, length);
	 } else if (1 == color) {
           gtk_text_insert (GTK_TEXT (termBox), fixed_font, &colorRed, NULL, text, length);
	 } else if (2 == color) {
           gtk_text_insert (GTK_TEXT (termBox), fixed_font, &colorGreen, NULL, text, length);
	}
         //gtk_text_insert (GTK_TEXT (termBox), NULL, &colour, NULL,
         //                 "colored ", -1);

         /* Thaw the text widget, allowing the updates to become visible */  
         gtk_text_thaw (GTK_TEXT (termBox));

	 // Now move the adjustment so the scrollbar and the
	 adj = GTK_TEXT(termBox)->vadj;
	 adj->value = adj->upper - adj->page_size;
	 gtk_adjustment_value_changed( adj );
      	 // printf("Upper->%0f ", GTK_ADJUSTMENT(widget)->upper);
}


      /* another callback */
      void delete_event( GtkWidget *widget,
                         GdkEvent  *event,
                         gpointer   data )
      {
          gtk_main_quit ();
      }

      int main( int   argc,
                char *argv[] )
      {
          int i;
	  char * module;
	  char buff[128];
	  char * p;

	  Proto environment;
	 Proto arguments;
	  
	  
	  int needsCompile;
	  
	  
          /* GtkWidget is the storage type for widgets */
          GtkWidget *window;
          GtkWidget *button;
          GtkWidget *box1;
          GtkWidget *vbox1;
          GtkWidget *hbox1;
          GtkWidget *vscrollbar;


          /* GtkWidget is the storage type for widgets */
          GtkWidget *window2;
          GtkWidget *window3;
          GtkWidget *vbox, *hbox;
          GtkWidget *vbox3;
          GtkWidget *scrolled_window;
          GtkWidget *scrolled_window3;
          GtkWidget *button_add, *button_clear, *button_hide_show;    
          gchar *titles[4] = { "Reg", "Address", "Value", "Data" };
          gchar *regtitles[3] = { "Reg", "Value", "Data" };

          /* This is called in all GTK applications. Arguments are parsed
           * from the command line and are returned to the application. */
          gtk_init (&argc, &argv);


          for (i = 1; i < argc; i++) {
	       if (argv[i][0] == '-') {
       		  if (strcmp(argv[i], "--trace") == 0) {
       		    //runtimeActivateTrace = 1;
       		    printf("debugTrace ON\n");
       		  }
       		} else {
       		  module = argv[i];
		  break;
       		}
          }


	// Load the module
        if (argc == 1) {
	  printf("Syntax: aqdbg <module file>\n\n");
	  exit(1);
        }



          /* Create a new window */
          MainWindow = window = gtk_window_new (GTK_WINDOW_TOPLEVEL);

          /* This is a new call, which just sets the title of our
           * new window to "Hello Buttons!" */
          gtk_window_set_title (GTK_WINDOW (window), "AQUA DEBUGGER");
          gtk_widget_set_usize (GTK_WIDGET (window), 450, 150);

          /* Here we just set a handler for delete_event that immediately
           * exits GTK. */
          gtk_signal_connect (GTK_OBJECT (window), "delete_event",
                              GTK_SIGNAL_FUNC (delete_event), NULL);

          /* Sets the border width of the window. */
          //gtk_container_set_border_width (GTK_CONTAINER (window), 10);

          /* We create a box to pack widgets into.  This is described in detail
           * in the "packing" section. The box is not really visible, it
           * is just used as a tool to arrange widgets. */
          box1 = gtk_hbox_new(FALSE, 0);
          vbox1 = gtk_vbox_new(FALSE, 0);
          hbox1 = gtk_hbox_new(FALSE, 0);

          /* Put the box into the main window. */
          gtk_container_add (GTK_CONTAINER (window), vbox1);
          gtk_box_pack_start(GTK_BOX(vbox1), box1, FALSE, FALSE, 0);


          gtk_container_add (GTK_CONTAINER (vbox1), hbox1);
	  termBox = gtk_text_new(NULL,NULL);
          gtk_widget_set_usize (GTK_WIDGET (termBox), 150, 150);
          gtk_box_pack_start(GTK_BOX(hbox1), termBox, TRUE, TRUE, 0);
          gtk_widget_show(termBox);

  	  vscrollbar = gtk_vscrollbar_new (GTK_TEXT(termBox)->vadj);
// Enable this to view the adjustment changes
//          gtk_signal_connect (GTK_OBJECT (GTK_TEXT(termBox)->vadj),
//	  		     "value_changed", GTK_SIGNAL_FUNC (adjCallback), NULL);
          gtk_box_pack_start(GTK_BOX(hbox1), vscrollbar, FALSE, FALSE, 0);
          gtk_widget_show (vscrollbar);


         /* Get the system color map and allocate the color red */
         cmap = gdk_colormap_get_system();
         colorRed.red = 0xffff;
         colorRed.green = 0;
         colorRed.blue = 0;
         if (!gdk_color_alloc(cmap, &colorRed)) {
           g_error("couldn't allocate color");
         }
         colorGreen.red = 0;
         colorGreen.green = 0x8080;
         colorGreen.blue = 0;
         if (!gdk_color_alloc(cmap, &colorGreen)) {
           g_error("couldn't allocate color");
         }


	
         /* Load a fixed font */
         fixed_font = gdk_font_load ("-*-lucidatypewriter-bold-r-*-*-*-140-*-*-*-*-*-*");
         fixed_font = gdk_font_load ("-*-terminal-*-*-*-*-*-*-*-*-*-*-iso-*");
         fixed_font = gdk_font_load ("-*-terminal-*-*-*-*-*-*-*-*-*-*-iso8859-*");




          /* Creates a new button with the label "Step". */
          button = gtk_button_new_with_label ("Step");

          /* Now when the button is clicked, we call the "callback" function
           * with a pointer to "Step" as its argument */
          gtk_signal_connect (GTK_OBJECT (button), "clicked",
                              GTK_SIGNAL_FUNC (callback), (gpointer) "step");

          /* Instead of gtk_container_add, we pack this button into the invisible
           * box, which has been packed into the window. */
          gtk_box_pack_start(GTK_BOX(box1), button, TRUE, TRUE, 0);

          /* Always remember this step, this tells GTK that our preparation for
           * this button is complete, and it can now be displayed. */
          gtk_widget_show(button);



          /* Do these same steps again to create a second button */
          button = gtk_toggle_button_new_with_label ("Trace");

          gtk_signal_connect (GTK_OBJECT (button), "toggled",
                              GTK_SIGNAL_FUNC (traceToggle), (gpointer) "Toggle");

          gtk_box_pack_start(GTK_BOX(box1), button, TRUE, TRUE, 0);
          gtk_widget_show(button);



          /* Do these same steps again to create another button */
          button = gtk_button_new_with_label ("Run");

          gtk_signal_connect (GTK_OBJECT (button), "clicked",
                              GTK_SIGNAL_FUNC (runCallback), (gpointer) "Run");

          gtk_box_pack_start(GTK_BOX(box1), button, TRUE, TRUE, 0);
          gtk_widget_show(button);





          button = gtk_button_new_with_label ("Dump");
          gtk_signal_connect (GTK_OBJECT (button), "clicked",
                              GTK_SIGNAL_FUNC (dumpCallback), (gpointer) "dump");

          gtk_box_pack_start(GTK_BOX(box1), button, TRUE, TRUE, 0);
          gtk_widget_show(button);


          button = gtk_button_new_with_label ("Break");
          gtk_signal_connect (GTK_OBJECT (button), "clicked",
                              GTK_SIGNAL_FUNC (breakCallback), (gpointer) "Break");
          gtk_box_pack_start(GTK_BOX(box1), button, TRUE, TRUE, 0);
          gtk_widget_show(button);



          /* Do these same steps again to create a second button */
          button = gtk_button_new_with_label ("Exit");

          gtk_signal_connect (GTK_OBJECT (button), "clicked",
                              GTK_SIGNAL_FUNC (callback), (gpointer) "Exit");
          gtk_signal_connect (GTK_OBJECT (button), "clicked",
                             GTK_SIGNAL_FUNC (delete_event), NULL);

          gtk_box_pack_start(GTK_BOX(box1), button, TRUE, TRUE, 0);
          gtk_widget_show(button);


	  textBox = gtk_entry_new_with_max_length( 40 );
          gtk_signal_connect (GTK_OBJECT (textBox), "activate",
                              GTK_SIGNAL_FUNC (comm_callback), (gpointer) textBox);
          gtk_box_pack_start(GTK_BOX(box1), textBox, TRUE, TRUE, 0);
          gtk_widget_show(textBox);


          /* The order in which we show the buttons is not really important, but I
           * recommend showing the window last, so it all pops up at once. */

          gtk_widget_show(box1);
          gtk_widget_show(vbox1);
          gtk_widget_show(hbox1);

          gtk_widget_show (window);
          gtk_widget_set_uposition (GTK_WIDGET (window), 15, 40);



          /* Create a new window */
          window2 = gtk_window_new (GTK_WINDOW_TOPLEVEL);

          gtk_window_set_title (GTK_WINDOW (window2), "STACK");

          gtk_widget_set_usize (GTK_WIDGET (window2), 480, 450);

          /* Here we just set a handler for delete_event that immediately
           * exits GTK. */
          //gtk_signal_connect (GTK_OBJECT (window2), "delete_event",
          //                    GTK_SIGNAL_FUNC (delete_event), NULL);

          /* Sets the border width of the window. */
          //gtk_container_set_border_width (GTK_CONTAINER (window2), 10);

          vbox = gtk_vbox_new(FALSE, 7);

          /* Put the box into the main window. */
          gtk_container_add (GTK_CONTAINER (window2), vbox);



          /* Create a scrolled window to pack the CList widget into */
          scrolled_window = gtk_scrolled_window_new (NULL, NULL);
          gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrolled_window),
                                          GTK_POLICY_AUTOMATIC, GTK_POLICY_ALWAYS);

          gtk_box_pack_start(GTK_BOX(vbox), scrolled_window, TRUE, TRUE, 0);
          gtk_widget_show (scrolled_window);

          /* Create the CList. For this example we use 4 columns */
          clist = gtk_clist_new_with_titles( 4, titles);

          /* When a selection is made, we want to know about it. The callback
           * used is selection_made, and its code can be found further down */
          gtk_signal_connect(GTK_OBJECT(clist), "select_row",
                             GTK_SIGNAL_FUNC(selection_made),
                             NULL);

          /* It isn't necessary to shadow the border, but it looks nice :) */
//          gtk_clist_set_shadow_type (GTK_CLIST(clist), GTK_SHADOW_OUT);

          /* What however is important, is that we set the column widths as
           * they will never be right otherwise. Note that the columns are
           * numbered from 0 and up (to 1 in this case).
           */
          gtk_clist_set_column_width (GTK_CLIST(clist), 0, 50);
          gtk_clist_set_column_width (GTK_CLIST(clist), 1, 150);
          gtk_clist_set_column_width (GTK_CLIST(clist), 2, 150);
          gtk_clist_set_column_width (GTK_CLIST(clist), 3, 150);

          /* Add the CList widget to the vertical box and show it. */
          gtk_container_add(GTK_CONTAINER(scrolled_window), clist);
          gtk_widget_show(clist);

          /* Create the buttons and add them to the window. See the button
           * tutorial for more examples and comments on this.
           */
          hbox = gtk_hbox_new(FALSE, 0);
          gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, TRUE, 0);
          gtk_widget_show(hbox);

          button_add = gtk_button_new_with_label("Add List");
          button_clear = gtk_button_new_with_label("Clear List");
          button_hide_show = gtk_button_new_with_label("Hide/Show titles");

          gtk_box_pack_start(GTK_BOX(hbox), button_add, TRUE, TRUE, 0);
          gtk_box_pack_start(GTK_BOX(hbox), button_clear, TRUE, TRUE, 0);
          gtk_box_pack_start(GTK_BOX(hbox), button_hide_show, TRUE, TRUE, 0);

          /* Connect our callbacks to the three buttons */
          gtk_signal_connect_object(GTK_OBJECT(button_add), "clicked",
                                    GTK_SIGNAL_FUNC(button_add_clicked),
                                    (gpointer) clist);
          gtk_signal_connect_object(GTK_OBJECT(button_clear), "clicked",
                                    GTK_SIGNAL_FUNC(button_clear_clicked),
                                    (gpointer) clist);
          gtk_signal_connect_object(GTK_OBJECT(button_hide_show), "clicked",
                                    GTK_SIGNAL_FUNC(button_hide_show_clicked),
                                    (gpointer) clist);


	  gtk_widget_show(button_add);
          gtk_widget_show(button_clear);
          gtk_widget_show(button_hide_show);


          gtk_widget_show (scrolled_window);
          gtk_widget_show (vbox);
          gtk_widget_show (window2);
          gtk_widget_set_uposition (GTK_WIDGET (window2), 480, 40);




          /* Create a new window */
          window3 = gtk_window_new (GTK_WINDOW_TOPLEVEL);

          gtk_window_set_title (GTK_WINDOW (window3), "REGS");

          gtk_widget_set_usize (GTK_WIDGET (window3), 350, 100);

          vbox3 = gtk_vbox_new(FALSE, 7);

          /* Put the box into the main window. */
          gtk_container_add (GTK_CONTAINER (window3), vbox3);


          /* Create a scrolled window to pack the CList widget into */
          scrolled_window3 = gtk_scrolled_window_new (NULL, NULL);
          gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrolled_window3),
                                          GTK_POLICY_AUTOMATIC, GTK_POLICY_ALWAYS);

          gtk_box_pack_start(GTK_BOX(vbox3), scrolled_window3, TRUE, TRUE, 0);
          gtk_widget_show (scrolled_window3);

          /* Create the CList. For this example we use 4 columns */
          reglist = gtk_clist_new_with_titles( 3, regtitles);

          gtk_clist_set_column_width (GTK_CLIST(reglist), 0, 50);
          gtk_clist_set_column_width (GTK_CLIST(reglist), 1, 150);
          gtk_clist_set_column_width (GTK_CLIST(reglist), 2, 150);

          gtk_container_add(GTK_CONTAINER(scrolled_window3), reglist);
          gtk_widget_show(reglist);

          gtk_widget_show (scrolled_window3);
          gtk_widget_show (vbox3);
          gtk_widget_show (window3);
          gtk_widget_set_uposition (GTK_WIDGET (window3), 480, 495);




          // Set the window title
	  buff[0] = 0;
	  strcat(buff, "Aqua Debug - ");
	  strcat(buff, module);
          gtk_window_set_title (GTK_WINDOW (window),buff); 


	  // Check if the module needs to be compiled to disk
//	  needsCompile = checkCompiled(filename);
	  needsCompile = 1;
	  
	  
    	  CPU = doCelInit(module, 1);
	  
	  ioOut = dbgIoOut;

    // Setup the Environment
    i = 0;

    environment = objectNew(0);
    arguments = createArray(nil);
    objectSetSlot(Capsule, stringToAtom("environment"), PROTO_ACTION_GET, (uint32) environment);
    objectSetSlot(Capsule, stringToAtom("arguments"), PROTO_ACTION_GET, (uint32) arguments);
    
    while (1) {
	if (environ[i] == NULL) {
	    break;
	}

	strcpy(buff, environ[i]);
	
	p = strchr(buff, '=');
	*p= 0;
	p++;
	
	objectSetSlot(environment, stringToAtom(buff), PROTO_ACTION_GET, (uint32) stringToAtom(p));
	i++;
    }
    
    for (i = 0; i < argc; i++) {
	//printf("%d - %s\n", i, argv[i]);
	addToArray(arguments, (Proto) stringToAtom(argv[i]));
    }


	  
          doDebuggerDisplay();
	  
          /* Rest in gtk_main and wait for the fun to begin! */
          gtk_main ();

          return(0);
      }

/*
  
  GtkStyle *style;
  
style = gtk_style_new();
label = gtk_label_new ("Quit");
gdk_font_unref (style->font);
style->font = gdk_font_load 
("-adobe-times-bold-i-normal--25-180-100-100-p-128-iso8859-1");
gtk_widget_set_style (label, style);
*/
