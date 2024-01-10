# -*- coding: utf-8 -*-
"""
Created on Thu Dec 14 12:09:09 2023

@author: Tomas
"""

import tkinter as tk
from tkinter import ttk
from matplotlib.backends.backend_tkagg import FigureCanvasTkAgg
from matplotlib.figure import Figure
import serial
import threading
import time

# Global variables
BAUD_RATE = 115200
COM_PORT = "COM6"
Y_MIN, Y_MAX = 20, 90
SAMPLES_PER_SECOND = 2

data_list = []

# Function to read data from the serial port
def read_serial_data(ser, stop_event):
    global data_list  # Use the global data_list

    while not stop_event.is_set():
        try:
            line = ser.readline().decode().strip()
            if line:
                try:
                    value = float(line) 
                    data_list.append(value)
                except ValueError:
                    pass
        except serial.SerialException:
            pass

# Function to update the plot
def update_plot(canvas, ax):
    ax.clear()
    ax.set_ylim(Y_MIN, Y_MAX)

    x = [i / SAMPLES_PER_SECOND for i in range(len(data_list))]
    ax.plot(x, data_list, color='b')

    canvas.draw()

# Function to update the instantaneous value label
def update_instantaneous_label(label):
    if data_list:
        value = data_list[-1]
        label.config(text=f'Instantaneous Value: {value:.2f} dB')
    else:
        label.config(text='Instantaneous Value: N/A')

# Main function to create and run the GUI
def main():
    # Initialize serial port
    ser = serial.Serial(COM_PORT, BAUD_RATE, timeout=1)

    # Initialize Tkinter
    root = tk.Tk()
    root.title("Sound Level Meter")

    # Create Matplotlib figure and axis
    figure = Figure(figsize=(6, 4), dpi=100)
    ax = figure.add_subplot(111)

    # Create Tkinter canvas
    canvas = FigureCanvasTkAgg(figure, master=root)
    canvas_widget = canvas.get_tk_widget()
    canvas_widget.grid(row=0, column=0, padx=10, pady=10)

    # Create label for instantaneous value
    instantaneous_label = tk.Label(root, text="Instantaneous Value: N/A")
    instantaneous_label.grid(row=1, column=0, padx=10, pady=10)

    # Create a thread to read data from the serial port
    stop_event = threading.Event()
    serial_thread = threading.Thread(target=read_serial_data, args=(ser, stop_event))
    serial_thread.start()

    # Update plot, label, and GUI periodically
    def update_gui():
        update_plot(canvas, ax)
        update_instantaneous_label(instantaneous_label)
        root.after(1000 // SAMPLES_PER_SECOND, update_gui)  # Update every 500 milliseconds

    # Call the update_gui function
    update_gui()

    # Handle window close event
    def on_close():
        stop_event.set()
        root.destroy()

    # Set the close event handler
    root.protocol("WM_DELETE_WINDOW", on_close)

    # Run the Tkinter main loop
    root.mainloop()

    # Close the serial port and wait for the thread to finish
    ser.close()
    serial_thread.join()

if __name__ == "__main__":
    main()
