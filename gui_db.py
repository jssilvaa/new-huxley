import sqlite3
import tkinter as tk
from tkinter import ttk, messagebox

DB_NAME = 'file:huxley.db?mode=ro'

def fetch_logs():
    """Fetches logs and updates the Treeview."""
    # Clear existing data
    for item in tree.get_children():
        tree.delete(item)

    conn = None
    try:
        conn = sqlite3.connect(DB_NAME, uri=True)
        cursor = conn.cursor()
        
        # 1. Get the last N logs
        N = 50
        cursor.execute(f"SELECT * FROM logs ORDER BY rowid DESC LIMIT {N}")
        rows = cursor.fetchall()
        
        # 2. Dynamic Column Setup (First run only)
        # We check if columns are set up by looking at the first column identifier
        if not tree["columns"] and cursor.description:
            columns = [description[0] for description in cursor.description]
            tree["columns"] = columns
            tree.column("#0", width=0, stretch=tk.NO) # Hide default "text" column
            
            for col in columns:
                tree.heading(col, text=col, anchor=tk.W)
                # Adjust width roughly based on name length, min 100px
                tree.column(col, width=max(100, len(col) * 10), anchor=tk.W)

        # 3. Insert Data
        for row in rows:
            tree.insert("", tk.END, values=row)

    except sqlite3.OperationalError as e:
        messagebox.showerror("Database Error", f"Error accessing DB:\n{e}")
    finally:
        if conn:
            conn.close()

# --- GUI Setup ---
root = tk.Tk()
root.title("Huxley DB Log Viewer")
root.geometry("800x400")

# Frame for the Table and Scrollbar
frame = ttk.Frame(root)
frame.pack(fill=tk.BOTH, expand=True, padx=10, pady=10)

# Scrollbar
scrollbar = ttk.Scrollbar(frame, orient=tk.VERTICAL)
scrollbar.pack(side=tk.RIGHT, fill=tk.Y)

# Treeview (The Table)
tree = ttk.Treeview(frame, yscrollcommand=scrollbar.set, selectmode="browse")
tree.pack(side=tk.LEFT, fill=tk.BOTH, expand=True)
scrollbar.config(command=tree.yview)

# Refresh Button
btn_refresh = ttk.Button(root, text="Refresh Logs", command=fetch_logs)
btn_refresh.pack(fill=tk.X, padx=10, pady=(0, 10))

# Initial Load
fetch_logs()

root.mainloop()