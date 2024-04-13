import tkinter
import customtkinter
from tkintermapview import TkinterMapView

customtkinter.set_default_color_theme("blue")

class CustomMapView(TkinterMapView):
    
    def __init__(self, app, *args, **kwargs):
        super().__init__(*args, **kwargs)
        self.app = app
      
    
    def mouse_right_click(self, event):
        coordinate_mouse_pos = self.convert_canvas_coords_to_decimal_coords(event.x, event.y)

        def click_coordinates_event():
            try:
                self.set_marker(coordinate_mouse_pos[0], coordinate_mouse_pos[1])
                tkinter.messagebox.showinfo(title="", message="Waypoint added!")
                app.update_waypoints_listbox()

            except Exception as err:
                tkinter.messagebox.showinfo(title="", message="Error adding waypoint to course: \n" + str(err))

        m = tkinter.Menu(self, tearoff=0)
        m.add_command(label=f"Add Waypoint", command=click_coordinates_event)

        m.tk_popup(event.x_root, event.y_root)


class App(customtkinter.CTk):

    APP_NAME = "AutoSail GPS Plotter"
    WIDTH = 1200
    HEIGHT = 800

    def __init__(self, *args, **kwargs):
        super().__init__(*args, **kwargs)

        self.title(App.APP_NAME)
        self.geometry(str(App.WIDTH) + "x" + str(App.HEIGHT))
        self.minsize(App.WIDTH, App.HEIGHT)

        self.protocol("WM_DELETE_WINDOW", self.on_closing)
        self.bind("<Control-q>", self.on_closing)
        self.bind("<Control-w>", self.on_closing)

        # ============ create two CTkFrames ============

        self.grid_columnconfigure(0, weight=0)
        self.grid_columnconfigure(1, weight=1)
        self.grid_rowconfigure(0, weight=1)

        self.infoFrame = customtkinter.CTkFrame(master=self, width=App.WIDTH/4, corner_radius=0, fg_color=None)
        self.infoFrame.grid(row=0, column=0, padx=0, pady=0, sticky="nsew")

        self.mapFrame = customtkinter.CTkFrame(master=self, corner_radius=0)
        self.mapFrame.grid(row=0, column=1, rowspan=1, pady=0, padx=0, sticky="nsew")

        # ============ info frame ============

        self.infoFrame.grid_rowconfigure(2, weight=1)

        self.clearWaypointsButton = customtkinter.CTkButton(master=self.infoFrame,
                                                text="Clear Waypoints",
                                                command=self.clearWaypoints)
        self.clearWaypointsButton.grid(pady=(20, 0), padx=(20, 20), row=3, column=0)

        self.waypoints_listbox = tkinter.Listbox(self.infoFrame)

        self.waypoints_listbox.grid(row=2, column=0, padx=(20, 20), pady=(20, 20), sticky="nsew")

        self.sendWaypointsButton = customtkinter.CTkButton(master=self.infoFrame,
                                                text="Send Waypoints to Vessel",
                                                command=self.sendWaypoints)

        self.sendWaypointsButton.grid(pady=(20, 0), padx=(20, 20), row=1, column=0)

        # ============ map frame ============

        self.mapFrame.grid_rowconfigure(1, weight=1)
        self.mapFrame.grid_rowconfigure(0, weight=0)
        self.mapFrame.grid_columnconfigure(0, weight=1)
        self.mapFrame.grid_columnconfigure(1, weight=0)
        self.mapFrame.grid_columnconfigure(2, weight=1)

        self.map_widget = CustomMapView(self, self.mapFrame, corner_radius=0)
        self.waypointList = self.map_widget.canvas_marker_list
        self.map_widget.grid(row=1, rowspan=1, column=0, columnspan=3, sticky="nswe", padx=(0, 0), pady=(0, 0))
        self.entry = customtkinter.CTkEntry(master=self.mapFrame,
                                            placeholder_text="Search Google Maps")
        self.entry.grid(row=0, column=0, sticky="we", padx=(12, 0), pady=12)
        self.entry.bind("<Return>", self.search_event)

        self.searchButton = customtkinter.CTkButton(master=self.mapFrame, text="Search", width=90, command=self.search_event)
        self.searchButton.grid(row=0, column=1, sticky="w", padx=(12, 0), pady=12)

        # Set default values
        self.map_widget.set_address("Wilmington, NC")
        self.map_widget.set_tile_server("https://mt0.google.com/vt/lyrs=s&hl=en&x={x}&y={y}&z={z}&s=Ga", max_zoom=22)

    def search_event(self, event=None):
        self.map_widget.set_address(self.entry.get())

    def update_waypoints_listbox(self):
        # Clear the listbox
        self.waypoints_listbox.delete(0, tkinter.END)

        # Add waypoints to the listbox
        for i, waypoint in enumerate(self.waypointList, start=1):
            lat, lon = waypoint.position
            self.waypoints_listbox.insert(tkinter.END, f"Waypoint {i}: {lat}, {lon}")

    def clearWaypoints(self):
        for waypoint in self.waypointList:
            waypoint.delete()
        self.waypointList.clear()

    def sendWaypoints(self):
        coordList = []
        for waypoint in self.waypointList:
            coordList.append(waypoint.position)

        # actually transmit coords here somehow

    def on_closing(self, event=0):
        self.destroy()

    def start(self):
        self.mainloop()


if __name__ == "__main__":
    app = App()
    app.start()
