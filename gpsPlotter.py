import time
import serial
import tkinter
import termios
import customtkinter
import serial.tools.list_ports
from tkintermapview import TkinterMapView
from tkintermapview.canvas_position_marker import CanvasPositionMarker

customtkinter.set_default_color_theme("dark-blue")

class CustomWaypointMarker(CanvasPositionMarker):
    def __init__(self, *args, **kwargs):
        super().__init__(*args, **kwargs)
        self.text_color = "#C5542D"

class CustomMapView(TkinterMapView):
    
    def __init__(self, app, *args, **kwargs):
        super().__init__(*args, **kwargs)
        self.app = app
        self.tile_server = "https://mt0.google.com/vt/lyrs=s&hl=en&x={x}&y={y}&z={z}&s=Ga"
    
    def mouse_right_click(self, event):
        coordinate_mouse_pos = self.convert_canvas_coords_to_decimal_coords(event.x, event.y)

        def click_coordinates_event():
            try:
                self.set_marker(coordinate_mouse_pos[0], coordinate_mouse_pos[1], text=(str(len(app.waypointList)+1)))
                app.update_waypoints_listbox()

            except Exception as err:
                tkinter.messagebox.showinfo(title="", message="Error adding waypoint to course: \n" + str(err))

        m = tkinter.Menu(self, tearoff=0)
        m.add_command(label=f"Add Waypoint", command=click_coordinates_event)

        m.tk_popup(event.x_root, event.y_root)
    
    def set_marker(self, deg_x: float, deg_y: float, text: str = None, **kwargs) -> CustomWaypointMarker:
        marker = CustomWaypointMarker(self, (deg_x, deg_y), text=text, **kwargs)
        marker.draw()
        self.canvas_marker_list.append(marker)
        return marker


class App(customtkinter.CTk):

    APP_NAME = "AutoSail GPS Plotter"
    WIDTH = 1400
    HEIGHT = 800

    def __init__(self, *args, **kwargs):
        super().__init__(*args, **kwargs)
        customtkinter.set_appearance_mode("dark")
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
        self.mapFrame.grid(row=0, column=1, rowspan=1, pady=(0, 20), padx=(0, 20), sticky="nsew")

        # ============ info frame ============

        self.infoFrame.grid_rowconfigure(3, weight=1)

        self.clearWaypointsButton = customtkinter.CTkButton(master=self.infoFrame,
                                                text="Clear Waypoints",
                                                command=self.clearWaypoints)
        self.clearWaypointsButton.grid(pady=(20, 20), padx=(20, 20), row=5, column=0)

        self.removeRecentWaypointButton = customtkinter.CTkButton(master=self.infoFrame,
                                                text="Remove Latest Waypoint",
                                                command=self.removeRecentWaypoint)
        self.removeRecentWaypointButton.grid(pady=(0, 0), padx=(20, 20), row=4, column=0)

        self.waypoints_listbox = tkinter.Listbox(self.infoFrame, fg="white", bg="black")

        self.waypoints_listbox.grid(row=3, column=0, padx=(20, 20), pady=(20, 20), sticky="nsew")

        self.sendWaypointsButton = customtkinter.CTkButton(master=self.infoFrame,
                                                text="Send Waypoints to Vessel",
                                                command=self.sendWaypoints)

        self.sendWaypointsButton.grid(pady=(20, 0), padx=(20, 20), row=2, column=0)
        self.serial_ports = ["No port selected"]
        self.port_option_menu = customtkinter.CTkOptionMenu(self.infoFrame, values=self.serial_ports, command=self.change_port)
        self.get_serial_ports(self.port_option_menu)
        self.serial_port = self.serial_ports[-1]
        self.serial_port_label = customtkinter.CTkLabel(self.infoFrame, text="Port:")
        self.serial_port_label.grid(pady=(20, 0), padx=(30, 0), row=1, column=0, sticky="w")
        self.port_option_menu.grid(row=1, column=0, padx=(0, 30), pady=(20, 0), sticky="e")

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

        self.instruction = customtkinter.CTkLabel(self.mapFrame, text="Right click to place waypoint")
        self.instruction.grid(row=0, column=2, padx=(10, 20), pady=12, sticky="ew")

        self.map_widget.set_address("Wilmington, NC")

    def get_serial_ports(self, port_option_menu):
        ports = [port.device for port in serial.tools.list_ports.comports()]
        ports.append("No port selected")
        self.serial_ports = ports
        port_option_menu.configure(values=self.serial_ports)
        self.after(1000, self.get_serial_ports, port_option_menu)

    def change_port(self, port):
        self.serial_port = port

    def search_event(self, event=None):
        self.map_widget.set_address(self.entry.get())

    def update_waypoints_listbox(self):
        self.waypoints_listbox.delete(0, tkinter.END)

        for i, waypoint in enumerate(self.waypointList, start=1):
            lat, lon = waypoint.position
            lat_str = "{:.5f}".format(lat)
            lon_str = "{:.5f}".format(lon)
            curWidth = 2*len(str(i)) + len(lat_str)+len(lon_str)
            padWidth = 23 - curWidth
            space = " "
            self.waypoints_listbox.insert(tkinter.END, f"{i}:{space*padWidth}{lat_str}, {lon_str}")

    def removeRecentWaypoint(self):
        if len(self.waypointList) != 0:
            self.waypointList[-1].delete()
            self.update_waypoints_listbox()

    def clearWaypoints(self):
        for i in range(len(self.waypointList) - 1, -1, -1):
            self.waypointList[i].delete()
        self.waypointList.clear()
        self.update_waypoints_listbox()

    def sendWaypoints(self):
        coordList = []
        for waypoint in self.waypointList:
            coordList.append(list(waypoint.position))

        #disable reset after serial disconnect
        with open(self.serial_port) as f:
            attrs = termios.tcgetattr(f)
            attrs[2] = attrs[2] & ~termios.HUPCL
            termios.tcsetattr(f, termios.TCSAFLUSH, attrs)

        serialCon = serial.Serial(self.serial_port, 115200)

        inData = ""
        while inData != "Ready":  #wait for the arduino to signal it is ready to recieve data
            inData = serialCon.readline().strip().decode()

        for coord in coordList:
            lat = "{:.8f}".format(coord[0])
            lon = "{:.8f}".format(coord[1])
            lat_lon_str = lat+','+lon+'\n'
            bytes = serialCon.write(lat_lon_str.encode())
        bytes = serialCon.write("␄\n".encode())
        serialCon.close()
        tkinter.messagebox.showinfo(title="", message="Waypoints sent!")

    def on_closing(self, event=0):
        self.destroy()

    def start(self):
        self.mainloop()


if __name__ == "__main__":
    app = App()
    app.start()
