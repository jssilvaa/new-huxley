#!/usr/bin/env python3
"""PyQt6 GUI for Huxley client."""
from __future__ import annotations

import sys
import os

from PyQt6.QtCore import Qt
from PyQt6.QtWidgets import (
    QApplication,
    QDialog,
    QHBoxLayout,
    QLabel,
    QLineEdit,
    QListWidget,
    QListWidgetItem,
    QMainWindow,
    QMessageBox,
    QPushButton,
    QSplitter,
    QVBoxLayout,
    QWidget,
    QFrame,
)

try:
    from .qt_session import ChatSession
except ImportError:
    sys.path.append(os.path.dirname(__file__))
    from qt_session import ChatSession  # type: ignore


HUXLEY_GREEN = "#44BBA4"
HUXLEY_DARK = "#1F1F1F"
HUXLEY_LIGHT = "#F5F7FA"
HUXLEY_GRAY = "#7A7A7A"
ME_BUBBLE = "#DCF8C6"  # WhatsApp-ish green
THEIR_BUBBLE = "#FFFFFF"


def circle_label(text: str, bg: str, fg: str = "#FFFFFF") -> QLabel:
    lbl = QLabel(text)
    lbl.setFixedSize(36, 36)
    lbl.setAlignment(Qt.AlignmentFlag.AlignCenter)
    lbl.setStyleSheet(
        f"""
        background-color: {bg};
        color: {fg};
        border-radius: 18px;
        font-weight: bold;
        """
    )
    return lbl


class LoginDialog(QDialog):
    def __init__(self) -> None:
        super().__init__()
        self.setWindowTitle("Huxley Login")
        self.username = ""
        self.password = ""

        layout = QVBoxLayout(self)
        layout.setContentsMargins(20, 20, 20, 20)

        # Logo
        logo_row = QHBoxLayout()
        logo = circle_label("H", HUXLEY_GREEN)
        logo_row.addWidget(logo, alignment=Qt.AlignmentFlag.AlignLeft)
        title = QLabel("HUXLEY")
        title.setStyleSheet("font-size: 20px; font-weight: 700;")
        logo_row.addWidget(title, alignment=Qt.AlignmentFlag.AlignLeft)
        logo_row.addStretch()
        layout.addLayout(logo_row)

        subtitle = QLabel("Login" if self.windowTitle().lower().startswith("huxley") else "Login")
        subtitle.setStyleSheet("font-size: 16px; font-weight: 600; margin-top: 8px;")
        layout.addWidget(subtitle)

        layout.addWidget(QLabel("Username:"))
        self.user_edit = QLineEdit()
        self.user_edit.setPlaceholderText("Enter username")
        layout.addWidget(self.user_edit)
        layout.addWidget(QLabel("Password:"))
        self.pass_edit = QLineEdit()
        self.pass_edit.setEchoMode(QLineEdit.EchoMode.Password)
        self.pass_edit.setPlaceholderText("Enter password")
        layout.addWidget(self.pass_edit)

        btn_row = QHBoxLayout()
        self.btn_login = QPushButton("Login")
        self.btn_register = QPushButton("Register")
        btn_row.addWidget(self.btn_login)
        btn_row.addWidget(self.btn_register)
        layout.addLayout(btn_row)
        self.setStyleSheet(
            f"""
            QDialog {{
                background: {HUXLEY_LIGHT};
            }}
            QLineEdit {{
                padding: 8px;
                border: 1px solid #ccc;
                border-radius: 6px;
            }}
            QPushButton {{
                padding: 8px 16px;
                border-radius: 6px;
                background: {HUXLEY_DARK};
                color: white;
            }}
            QPushButton:hover {{
                background: #2b2b2b;
            }}
            """
        )

        self.btn_login.clicked.connect(self.accept)
        self.btn_register.clicked.connect(self.reject)  # we'll signal register via result code

    def get_creds(self) -> tuple[str, str, bool]:
        return self.user_edit.text().strip(), self.pass_edit.text(), self.result() == QDialog.DialogCode.Rejected


class ChatBubble(QWidget):
    def __init__(self, text: str, mine: bool, timestamp: str) -> None:
        super().__init__()
        layout = QVBoxLayout(self)
        layout.setContentsMargins(0, 0, 0, 0)
        bubble = QLabel(text)
        bubble.setWordWrap(True)
        bubble.setStyleSheet(
            f"""
            QLabel {{
                background: {'%s' % (ME_BUBBLE if mine else THEIR_BUBBLE)};
                border-radius: 10px;
                padding: 8px;
                color: #000;
            }}
            """
        )
        stamp = QLabel(timestamp)
        stamp.setStyleSheet("color: #888; font-size: 10px;")
        stamp.setAlignment(Qt.AlignmentFlag.AlignRight)
        layout.addWidget(bubble)
        layout.addWidget(stamp)


class UserItemWidget(QWidget):
    def __init__(self, name: str, online: bool, unread: int) -> None:
        super().__init__()
        h = QHBoxLayout(self)
        h.setContentsMargins(6, 4, 6, 4)
        avatar = circle_label(name[:2].upper(), HUXLEY_GREEN if online else "#CCCCCC")
        h.addWidget(avatar)
        v = QVBoxLayout()
        v.setContentsMargins(6, 0, 0, 0)
        lbl = QLabel(name)
        lbl.setStyleSheet("font-weight: 600;")
        status = QLabel("online" if online else "offline")
        status.setStyleSheet(f"color: {'#0a0' if online else '#999'}; font-size: 10px;")
        v.addWidget(lbl)
        v.addWidget(status)
        h.addLayout(v)
        h.addStretch()
        if unread > 0:
            badge = QLabel(str(unread))
            badge.setFixedSize(22, 22)
            badge.setAlignment(Qt.AlignmentFlag.AlignCenter)
            badge.setStyleSheet(
                f"background:{HUXLEY_GREEN}; color:white; border-radius:11px; font-weight:bold;"
            )
            h.addWidget(badge)


class MainWindow(QMainWindow):
    def __init__(self, session: ChatSession) -> None:
        super().__init__()
        self.session = session
        self.selected_peer: str | None = None

        self.setWindowTitle("Huxley Chat")
        self.resize(1000, 700)

        self.user_list = QListWidget()
        self.chat_view = QListWidget()
        self.chat_view.setWordWrap(True)
        self.chat_view.setSpacing(4)
        self.chat_view.setUniformItemSizes(False)
        self.input = QLineEdit()
        self.btn_send = QPushButton("Send")

        splitter = QSplitter()
        splitter.addWidget(self.user_list)
        splitter.addWidget(self.chat_view)
        splitter.setStretchFactor(1, 3)
        splitter.setChildrenCollapsible(False)

        bottom = QWidget()
        h = QHBoxLayout(bottom)
        h.setContentsMargins(0, 0, 0, 0)
        h.addWidget(self.input)
        h.addWidget(self.btn_send)
        bottom.setFixedHeight(56)

        central = QWidget()
        v = QVBoxLayout(central)
        v.setContentsMargins(6, 6, 6, 6)
        v.addWidget(splitter)
        v.addWidget(bottom)
        v.setStretch(0, 1)
        v.setStretch(1, 0)
        self.setCentralWidget(central)

        self.setStyleSheet(
            f"""
            QListWidget {{
                background: {HUXLEY_LIGHT};
            }}
            QLineEdit {{
                padding: 10px;
                border: 1px solid #ccc;
                border-radius: 8px;
            }}
            QPushButton {{
                padding: 10px 16px;
                background: {HUXLEY_GREEN};
                color: white;
                border: none;
                border-radius: 8px;
                font-weight: 600;
            }}
            QPushButton:hover {{
                background: #3ca78f;
            }}
            """
        )

        self.user_list.currentItemChanged.connect(self.on_user_selected)
        self.btn_send.clicked.connect(self.on_send)
        self.input.returnPressed.connect(self.on_send)

        # Wire signals
        session.users_updated.connect(self.update_users)
        session.conversation_updated.connect(self.update_conversation)
        session.status_changed.connect(self.show_status_msg)
        session.error_occurred.connect(self.show_error_msg)
        session.unread_updated.connect(self.update_unread)
        session.disconnected.connect(self.handle_disconnect)

        self.unread: dict[str, int] = {}

    # Slots -----------------------------------------------------------------
    def update_users(self, users: list[dict]) -> None:
        current = self.selected_peer
        self.user_list.blockSignals(True)
        self.user_list.clear()
        sorted_users = sorted(
            [u for u in users if u.get("username") != (self.session.message_client.username or "")],
            key=lambda u: (not u.get("online", False), u.get("username", "")),
        )
        for u in sorted_users:
            name = u.get("username", "?")
            online = u.get("online") is True
            unread = self.unread.get(name, 0)
            item = QListWidgetItem()
            item.setData(Qt.ItemDataRole.UserRole, name)
            widget = UserItemWidget(name, online, unread)
            item.setSizeHint(widget.sizeHint())
            self.user_list.addItem(item)
            self.user_list.setItemWidget(item, widget)
            if name == current:
                self.user_list.setCurrentItem(item)
        self.user_list.blockSignals(False)

    def update_unread(self, unread: dict) -> None:
        self.unread = unread
        # force refresh labels
        items = []
        for idx in range(self.user_list.count()):
            items.append(self.user_list.item(idx))
        for item in items:
            name = item.data(Qt.ItemDataRole.UserRole)
            count = self.unread.get(name, 0)
            text = item.text().split("  [")[0]
            if count:
                text = f"{text.split(' [')[0]}  [{count}]"
            item.setText(text)

    def update_conversation(self, peer: str, messages: list[dict]) -> None:
        if peer != self.selected_peer:
            return
        self.chat_view.clear()
        for m in messages:
            ts = m.get("timestamp", "")
            sender = m.get("from", "")
            content = m.get("content", "")
            mine = sender == (self.session.message_client.username or "")
            widget = ChatBubble(content, mine, ts)
            item = QListWidgetItem()
            item.setSizeHint(widget.sizeHint())
            if mine:
                item.setTextAlignment(Qt.AlignmentFlag.AlignRight)
            self.chat_view.addItem(item)
            self.chat_view.setItemWidget(item, widget)
        self.chat_view.scrollToBottom()

    def on_user_selected(self, current: QListWidgetItem, _prev: QListWidgetItem) -> None:
        if not current:
            return
        peer = current.data(Qt.ItemDataRole.UserRole)
        self.selected_peer = peer
        self.session.select_peer(peer)

    def on_send(self) -> None:
        if not self.selected_peer:
            return
        text = self.input.text().strip()
        if not text:
            return
        self.input.clear()
        self.session.send_message(self.selected_peer, text)

    def show_status_msg(self, msg: str) -> None:
        self.statusBar().showMessage(msg, 5000)

    def show_error_msg(self, msg: str) -> None:
        QMessageBox.critical(self, "Error", msg)

    def handle_disconnect(self) -> None:
        QMessageBox.warning(self, "Disconnected", "Server closed the connection.")
        self.close()

    def closeEvent(self, event) -> None:
        try:
            self.session.logout()
        except Exception:
            pass
        self.session.stop()
        super().closeEvent(event)


def main() -> int:
    import argparse

    parser = argparse.ArgumentParser(description="Huxley PyQt6 GUI")
    parser.add_argument("host", type=str, help="Server hostname or IP")
    parser.add_argument("port", type=int, help="Server port")
    parser.add_argument("--timeout", type=float, default=5.0)
    parser.add_argument("--history-limit", type=int, default=50)
    args = parser.parse_args()

    app = QApplication(sys.argv)

    session = ChatSession(args.host, args.port, args.timeout, history_limit=args.history_limit)
    if not session.connect_socket():
        return 1

    def attempt_login() -> bool:
        while True:
            dlg = LoginDialog()
            result = dlg.exec()
            username, password, wants_register = dlg.get_creds()
            if result == QDialog.DialogCode.Rejected and not wants_register:
                return False
            if wants_register:
                resp = session.message_client.register(username, password)
                if not resp or not resp.get("success"):
                    QMessageBox.critical(None, "Register failed", resp.get("message", "Registration failed") if resp else "No response")
                    continue
            resp = session.message_client.login(username, password)
            if resp and resp.get("success"):
                session.status_changed.emit("Logged in")
                return True
            QMessageBox.critical(None, "Login failed", resp.get("message", "Login failed") if resp else "No response")
            # retry loop

    if not attempt_login():
        return 0

    session.start()

    win = MainWindow(session)
    win.show()
    rc = app.exec()
    session.stop()
    return rc


if __name__ == "__main__":
    sys.exit(main())
