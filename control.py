import serial
import tkinter as tk
import time

# Setup Serial
try:
    ser = serial.Serial('COM4', 9600, timeout=0.1)
except:
    raise Exception("Could not open COM4. Check your connection.")

root = tk.Tk()
root.title("Smooth 6-DOF Control")

# Track the last sent value to avoid redundant commands
last_sent = [90] * 6
last_update_time = 0

def update_servo(index, val):
    global last_update_time
    current_time = time.time()
    
    # Only send data every 50ms (20Hz) to prevent Serial overflow
    if current_time - last_update_time > 0.05:
        angle = int(val)
        if angle != last_sent[index]:
            command = f"{index},{angle}\n"
            ser.write(command.encode())
            last_sent[index] = angle
            last_update_time = current_time
            print(f"Sent: {command.strip()}")  # Debug print

for i in range(6):
    tk.Label(root, text=f"Servo {i}").pack()
    # 'command' in Scale calls the function every time it moves
    s = tk.Scale(root, from_=0, to=180, orient="horizontal", 
                 length=300, command=lambda val, i=i: update_servo(i, val))
    s.set(90)
    s.pack(pady=5)

root.mainloop()