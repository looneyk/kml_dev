//------------------------------------------------------------------------------
// main.cpp
//------------------------------------------------------------------------------

#include <gtk/gtk.h>
#include "Camera.hpp"
#include "Lepton_I2C.hpp"

static const char* device = "/dev/spidev0.1";

//------------------------------------------------------------------------------
class FrameReader
{
public:

    //--------------------------------------------------------------------------
    FrameReader(const std::string& device, GdkPixbuf* ppixbuf, GtkWidget* pimage) :
        m_camera(device),
        m_frame(),
        m_ppixbuf(ppixbuf),
        m_pimage(pimage)
    {
    }

    Camera m_camera;
    Frame m_frame;
    GdkPixbuf* m_ppixbuf;
    GtkWidget* m_pimage;
};

//------------------------------------------------------------------------------
gint read_frame(gpointer pdata)
{
    assert(pdata != 0);

    FrameReader* fr = reinterpret_cast<FrameReader*>(pdata);

    fr.m_camera.capture(fr.m_frame);

    // assert same dimensions

    guchar* ppixels = gdk_pixbuf_get_pixels(ppixbuf);
    assert(ppixels != 0);
    memcpy(ppixels, fr.m_frame.pixels(), fr.m_frame.pixels().size() * sizeof Frame::pixel_type); // dangerous in general ...
    gtk_image_set_from_pixbuf(pimage, ppixbuf);
    gtk_widget_queue_draw(pimage);

    return TRUE;
}

//------------------------------------------------------------------------------
gint on_clicked_ffc(GtkWidget* pwidget, GdkEventButton* pevent)
{
    if (pevent->button == 1)
    {
        lepton_perform_ffc();
    }

    return TRUE;
}

//------------------------------------------------------------------------------
gint on_clicked_zzz(GtkWidget* pwidget, GdkEventButton* pevent)
{
    // ...

    return TRUE;
}

//------------------------------------------------------------------------------
int main(int argc, char **argv)
{
    gtk_init(&argc, &argv);

    // Main window
    GtkWidget* pwindow = gtk_window_new(GTK_WINDOW_TOPLEVEL); // GTK_WINDOW_TOPLEVEL = Has a titlebar and border, managed by the window manager.
    gtk_window_set_title(GTK_WINDOW(pwindow), "Lepton Video");
    gtk_window_set_default_size(GTK_WINDOW(pwindow), 400, 300); // Size of the the client area (excluding the additional areas provided by the window manager)
    gtk_window_set_position(GTK_WINDOW(pwindow), GTK_WIN_POS_CENTER);
    g_signal_connect(G_OBJECT(pwindow), "destroy", G_CALLBACK(gtk_main_quit), NULL); // Close button

    // Horizontal box
    GtkWidget* pbox = gtk_button_box_new(GTK_ORIENTATION_HORIZONTAL);
    gtk_container_set_border_width(GTK_CONTAINER(pbox), 10);
    gtk_button_box_set_layout(GTK_BUTTON_BOX(pbox), GTK_BUTTONBOX_SPREAD);
    gtk_container_add(GTK_CONTAINER(pwindow), pbox);

    // Frame
    GtkWidget* pframe = gtk_aspect_frame_new(NULL, .5, .5, 1., TRUE);
    gtk_widget_set_size_request(pframe, 100, 80);
    gtk_widget_set_halign(pframe, GTK_ALIGN_CENTER);
    gtk_widget_set_valign(pframe, GTK_ALIGN_CENTER);
    gtk_container_add(GTK_CONTAINER(pbox), pframe);

    // Video image
    GdkPixbuf* ppixbuf = gdk_pixbuf_new(GDK_COLORSPACE_RGB, FALSE, 8, 80, 60);
    GtkWidget* pimage = gtk_image_new_from_pixbuf(ppixbuf);
//    gtk_signal_connect(GTK_OBJECT(pimage), "expose_event", (GtkSignalFunc)on_expose_image, NULL);
//    gtk_signal_connect(GTK_OBJECT(pimage), "configure_event", (GtkSignalFunc)on_configure_image, NULL);
    gtk_container_add(GTK_CONTAINER(pframe), pimage);

    // Vertical button box
    GtkWidget* pbuttonbox = gtk_button_box_new(GTK_ORIENTATION_VERTICAL);
    gtk_button_box_set_layout(GTK_BUTTON_BOX(pbuttonbox), GTK_BUTTONBOX_START);
    gtk_box_set_spacing(GTK_BOX(pbuttonbox), 10);
    gtk_container_set_border_width(GTK_CONTAINER(pbuttonbox), 10);
    gtk_container_add(GTK_CONTAINER(pbox), pbuttonbox);

    // FFC button
    GtkWidget* pbutton = gtk_button_new_with_label("FFC");
    g_signal_connect(pbutton, "clicked", G_CALLBACK(on_clicked_ffc), NULL);
    gtk_container_add(GTK_CONTAINER(pbuttonbox), pbutton);

    // zzz button
    pbutton = gtk_button_new_with_label("zzz");
    g_signal_connect(pbutton, "clicked", G_CALLBACK(on_clicked_zzz), NULL);
    gtk_container_add(GTK_CONTAINER(pbuttonbox), pbutton);

    // Set up function to be periodically called to read frames
    FrameReader fr(device);
    guint id = gdk_threads_add_timeout(20, read_frame, &fr); 

    gtk_widget_show_all(pwindow);

    gtk_main();

    return 0;
}
