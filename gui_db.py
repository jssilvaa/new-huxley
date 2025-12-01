import sqlite3
import tkinter as tk
from tkinter import ttk, messagebox

DB_NAME = 'file:huxley.db?mode=ro'


class TableView(ttk.Frame):
    """Reusable table with a refresh callback."""

    def __init__(self, master, title, fetch_callback):
        super().__init__(master)
        self.fetch_callback = fetch_callback
        self.columns_set = False

        title_lbl = ttk.Label(self, text=title, font=("TkDefaultFont", 10, "bold"))
        title_lbl.pack(anchor=tk.W, padx=4, pady=2)

        frame = ttk.Frame(self)
        frame.pack(fill=tk.BOTH, expand=True)

        scrollbar = ttk.Scrollbar(frame, orient=tk.VERTICAL)
        scrollbar.pack(side=tk.RIGHT, fill=tk.Y)

        self.tree = ttk.Treeview(frame, yscrollcommand=scrollbar.set, selectmode="browse")
        self.tree.pack(side=tk.LEFT, fill=tk.BOTH, expand=True)
        scrollbar.config(command=self.tree.yview)

        btn_refresh = ttk.Button(self, text="Refresh", command=self.refresh)
        btn_refresh.pack(fill=tk.X, padx=2, pady=2)

    def refresh(self):
        try:
            rows, description = self.fetch_callback()
        except sqlite3.OperationalError as e:
            messagebox.showerror("Database Error", f"Error accessing DB:\n{e}")
            return

        for item in self.tree.get_children():
            self.tree.delete(item)

        if not self.columns_set and description:
            columns = [desc[0] for desc in description]
            self.tree["columns"] = columns
            self.tree.column("#0", width=0, stretch=tk.NO)
            for col in columns:
                self.tree.heading(col, text=col, anchor=tk.W)
                self.tree.column(col, width=max(100, len(col) * 10), anchor=tk.W)
            self.columns_set = True

        for row in rows:
            self.tree.insert("", tk.END, values=row)


def fetch_logs(conn, limit=100):
    cur = conn.cursor()
    cur.execute("SELECT * FROM logs ORDER BY rowid DESC LIMIT ?", (limit,))
    rows = cur.fetchall()
    return rows, cur.description


def fetch_users(conn):
    cur = conn.cursor()
    cur.execute("SELECT id, username, created_at FROM users ORDER BY username;")
    rows = cur.fetchall()
    return rows, cur.description


def fetch_undelivered(conn):
    cur = conn.cursor()
    cur.execute(
        """
        SELECT m.id, u_from.username AS sender, u_to.username AS recipient,
               m.timestamp, m.delivered
        FROM messages m
        JOIN users u_from ON u_from.id = m.sender_id
        JOIN users u_to   ON u_to.id   = m.recipient_id
        WHERE m.delivered = 0
        ORDER BY m.timestamp ASC, m.id ASC;
        """
    )
    rows = cur.fetchall()
    return rows, cur.description


def fetch_messages(conn, peer, limit=100):
    cur = conn.cursor()
    cur.execute(
        """
        SELECT m.id, u_from.username AS sender, u_to.username AS recipient,
               m.timestamp, m.delivered
        FROM messages m
        JOIN users u_from ON u_from.id = m.sender_id
        JOIN users u_to   ON u_to.id   = m.recipient_id
        WHERE u_from.username = ? OR u_to.username = ?
        ORDER BY m.timestamp DESC, m.id DESC
        LIMIT ?;
        """,
        (peer, peer, limit),
    )
    rows = cur.fetchall()
    return rows, cur.description


def main():
    root = tk.Tk()
    root.title("Huxley DB Inspector")
    root.geometry("900x600")

    conn = sqlite3.connect(DB_NAME, uri=True)

    notebook = ttk.Notebook(root)
    notebook.pack(fill=tk.BOTH, expand=True, padx=6, pady=6)

    logs_view = TableView(notebook, "Logs (latest)", lambda: fetch_logs(conn, limit=200))
    users_view = TableView(notebook, "Users", lambda: fetch_users(conn))
    undeliv_view = TableView(notebook, "Undelivered", lambda: fetch_undelivered(conn))

    messages_frame = ttk.Frame(notebook)
    top_bar = ttk.Frame(messages_frame)
    top_bar.pack(fill=tk.X, padx=4, pady=4)
    ttk.Label(top_bar, text="Peer username:").pack(side=tk.LEFT)
    peer_var = tk.StringVar()
    peer_entry = ttk.Entry(top_bar, textvariable=peer_var, width=20)
    peer_entry.pack(side=tk.LEFT, padx=4)
    msg_view = TableView(messages_frame, "Messages (latest)", lambda: fetch_messages(conn, peer_var.get(), 200))
    msg_view.pack(fill=tk.BOTH, expand=True)
    ttk.Button(top_bar, text="Load", command=msg_view.refresh).pack(side=tk.LEFT, padx=4)

    for view, title in [
        (logs_view, "Logs"),
        (users_view, "Users"),
        (undeliv_view, "Undelivered"),
        (messages_frame, "Messages"),
    ]:
        notebook.add(view, text=title)

    # Initial load
    logs_view.refresh()
    users_view.refresh()
    undeliv_view.refresh()

    root.mainloop()
    conn.close()


if __name__ == "__main__":
    main()
