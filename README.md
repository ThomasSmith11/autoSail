# Autonomous Boat Navigation System

## Overview
This project is aimed at developing an autonomous sailboat navigation system using Arduino and Python. The system enables the boat to navigate along a path of set waypoints autonomously.  Those waypoints can be selected graphically with a python app and uploaded to the vessel prior to launch.

## Components I Used
- **RC Laser**: https://rclaser.org/
  
- **Sail Winch Servo**: [https://www.amazon.com/Hitec-RCD-33785S-HS-785HB-Winch](https://www.amazon.com/Hitec-RCD-33785S-HS-785HB-Winch/dp/B000BOGI7E?pd_rd_w=ad2Bc&content-id=amzn1.sym.62bf6161-8bd0-4857-85d6-e30435da42bd&pf_rd_p=62bf6161-8bd0-4857-85d6-e30435da42bd&pf_rd_r=GWPG70EPB0B76EVHYJ84&pd_rd_wg=3RyLY&pd_rd_r=384cec8f-5281-4d6f-8800-75d6cd009fef&pd_rd_i=B000BOGI7E&psc=1&ref_=pd_bap_d_grid_rp_0_1_ec_pd_nav_hcs_rp_1_t)

- **Wind Direction Sensor**: [https://www.amazon.com/Direction-Sensor-Supply-Voltage-Output](https://www.amazon.com/Direction-Sensor-Supply-Voltage-Output/dp/B07GP154HP?pd_rd_w=850SM&content-id=amzn1.sym.62bf6161-8bd0-4857-85d6-e30435da42bd&pf_rd_p=62bf6161-8bd0-4857-85d6-e30435da42bd&pf_rd_r=4Y0DY95VD66DSPYFM713&pd_rd_wg=un1oC&pd_rd_r=9e0a96e1-d398-4c91-83bd-4b0a2294d328&pd_rd_i=B07GP154HP&ref_=pd_bap_d_grid_rp_0_1_ec_pd_nav_hcs_rp_4_t&th=1)

- **GPS Sensor**: [https://www.amazon.com/Navigation-Positioning-Microcontroller-Compatible-Sensitivity](https://www.amazon.com/Navigation-Positioning-Microcontroller-Compatible-Sensitivity/dp/B0B31NRSD2?pd_rd_w=cCZIo&content-id=amzn1.sym.62bf6161-8bd0-4857-85d6-e30435da42bd&pf_rd_p=62bf6161-8bd0-4857-85d6-e30435da42bd&pf_rd_r=CD36R4VXM8F2VCPHT5RK&pd_rd_wg=XoOs8&pd_rd_r=6a581f4a-4e49-4333-8fbe-f9a11691ae29&pd_rd_i=B0B31NRSD2&psc=1&ref_=pd_bap_d_grid_rp_0_1_ec_pd_nav_hcs_rp_2_t)

## Dependencies
- **Arduino IDE**: Required to upload the Arduino code to the microcontroller onboard the boat.
  
- **Python**: The Python application requires Python installed on the user's system. It also depends on several libraries present in Coordinate_Plotter/requirements.txt.

## Installation
1. Clone the repository to your local machine.
`git clone https://github.com/ThomasSmith11/autoSail.git`

2. Upload the Arduino code (`autoSail.ino`) to your Arduino board using the Arduino IDE.

3. Install the required Python dependencies.
`pip install Coordinate_Plotter/requirements.txt`


## Usage
1. Replace sail winch servo with purchased one, and install 3d printed wind direction sensor mount (found [here](Supplementary_Files)) on the bow of the boat.

2. Connect the Arduino board to your boat's hardware using the wiring diagram [here](Supplementary_Files/autoSailWiring.png).

3. Ensure that the boat is powered on.

4. Launch the Python application by running `python3 Coordinate_Plotter/gpsPlotter.py` and connect the arduino via USB.

5. Follow the on-screen instructions to select waypoints and send them to the boat.

6. Launch the boat, navigate to a safe location, then switch from manual control to autonomous control by pushing the left stick all the way to the right. You may regain manual control at any time by pushing the stick all the way to the left.

## License
This project is licensed under the MIT [License](LICENSE)

## Acknowledgments
- This project was inspired by the desire to explore autonomous navigation for wind powered vessels.
- Special thanks to the Arduino forum for their support and to @TomSchimansky for making the [map view](https://github.com/TomSchimansky/TkinterMapView) for tkinter.

