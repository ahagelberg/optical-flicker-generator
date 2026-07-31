#!/usr/bin/env python3
"""Minimal mDNS browser for optical flicker generators (_http._tcp.local)."""

import queue
import threading
import webbrowser
import tkinter as tk
from tkinter import ttk

from zeroconf import ServiceBrowser, ServiceStateChange, Zeroconf

SERVICE_TYPE = "_http._tcp.local."
HOSTNAME_PREFIX = "flicker-"
SERVICE_INSTANCE_PREFIX = "Flicker"
BROWSE_TIMEOUT_S = 3
QUEUE_POLL_MS = 100
WINDOW_TITLE = "Flicker Discovery"
WINDOW_WIDTH = 520
WINDOW_HEIGHT = 320
COL_HOSTNAME = "hostname"
COL_IP = "ip"
COL_PORT = "port"


def service_instance_name(full_name: str) -> str:
    marker = "._http"
    if marker in full_name:
        return full_name.split(marker, 1)[0]
    return full_name.rstrip(".")


def is_flicker_device(info, full_service_name: str) -> bool:
    if info is None:
        return False
    host = info.server.rstrip(".").lower()
    if host.startswith(HOSTNAME_PREFIX):
        return True
    return service_instance_name(full_service_name).startswith(SERVICE_INSTANCE_PREFIX)


def pick_ipv4(info) -> str:
    for addr in info.parsed_addresses():
        if "." in addr and ":" not in addr:
            return addr
    addrs = info.parsed_addresses()
    return addrs[0] if addrs else ""


def device_from_info(info, full_service_name: str) -> dict | None:
    if not is_flicker_device(info, full_service_name):
        return None
    hostname = info.server.rstrip(".")
    ip = pick_ipv4(info)
    if not hostname and not ip:
        return None
    return {
        COL_HOSTNAME: hostname,
        COL_IP: ip,
        COL_PORT: str(info.port),
    }


class FlickerDiscoveryApp:
    def __init__(self, root: tk.Tk) -> None:
        self.root = root
        self.root.title(WINDOW_TITLE)
        self.root.geometry(f"{WINDOW_WIDTH}x{WINDOW_HEIGHT}")
        self.root.minsize(WINDOW_WIDTH, WINDOW_HEIGHT)
        self.event_queue: queue.Queue = queue.Queue()
        self.devices_by_host: dict[str, dict] = {}
        self.browse_thread: threading.Thread | None = None
        self.scanning = False
        self._build_ui()
        self.root.after(QUEUE_POLL_MS, self._poll_queue)
        self.refresh()

    def _build_ui(self) -> None:
        toolbar = ttk.Frame(self.root, padding=(8, 8, 8, 4))
        toolbar.pack(fill=tk.X)
        self.refresh_btn = ttk.Button(toolbar, text="Refresh", command=self.refresh)
        self.refresh_btn.pack(side=tk.LEFT)
        self.open_btn = ttk.Button(toolbar, text="Open", command=self.open_selected)
        self.open_btn.pack(side=tk.LEFT, padx=(8, 0))
        self.status_var = tk.StringVar(value="Ready")
        ttk.Label(toolbar, textvariable=self.status_var).pack(side=tk.RIGHT)
        columns = (COL_HOSTNAME, COL_IP, COL_PORT)
        tree_frame = ttk.Frame(self.root, padding=(8, 4, 8, 8))
        tree_frame.pack(fill=tk.BOTH, expand=True)
        self.tree = ttk.Treeview(tree_frame, columns=columns, show="headings", selectmode="browse")
        self.tree.heading(COL_HOSTNAME, text="Hostname")
        self.tree.heading(COL_IP, text="IP")
        self.tree.heading(COL_PORT, text="Port")
        self.tree.column(COL_HOSTNAME, width=220, anchor=tk.W)
        self.tree.column(COL_IP, width=140, anchor=tk.W)
        self.tree.column(COL_PORT, width=60, anchor=tk.CENTER)
        scroll = ttk.Scrollbar(tree_frame, orient=tk.VERTICAL, command=self.tree.yview)
        self.tree.configure(yscrollcommand=scroll.set)
        self.tree.pack(side=tk.LEFT, fill=tk.BOTH, expand=True)
        scroll.pack(side=tk.RIGHT, fill=tk.Y)
        self.tree.bind("<Double-1>", lambda _e: self.open_selected())

    def refresh(self) -> None:
        if self.scanning:
            return
        self.scanning = True
        self.refresh_btn.state(["disabled"])
        self.devices_by_host.clear()
        for item in self.tree.get_children():
            self.tree.delete(item)
        self.status_var.set("Scanning...")
        self.browse_thread = threading.Thread(target=self._browse_worker, daemon=True)
        self.browse_thread.start()

    def _browse_worker(self) -> None:
        zc = None
        browser = None
        try:
            zc = Zeroconf()
            browser = ServiceBrowser(zc, SERVICE_TYPE, handlers=[self._on_service_event])
            threading.Event().wait(timeout=BROWSE_TIMEOUT_S)
        except Exception as exc:
            self.event_queue.put(("error", str(exc)))
        finally:
            if browser is not None:
                browser.cancel()
            if zc is not None:
                zc.close()
            self.event_queue.put(("done", None))

    def _on_service_event(
        self,
        zeroconf: Zeroconf,
        service_type: str,
        name: str,
        state_change: ServiceStateChange,
    ) -> None:
        if state_change is ServiceStateChange.Removed:
            return
        info = zeroconf.get_service_info(service_type, name, timeout=2000)
        device = device_from_info(info, name)
        if device is not None:
            self.event_queue.put(("add", device))

    def _poll_queue(self) -> None:
        while True:
            try:
                kind, payload = self.event_queue.get_nowait()
            except queue.Empty:
                break
            if kind == "add":
                self._add_device(payload)
            elif kind == "error":
                self.status_var.set(f"Error: {payload}")
            elif kind == "done":
                self.scanning = False
                self.refresh_btn.state(["!disabled"])
                count = len(self.devices_by_host)
                self.status_var.set(f"{count} device(s)")
        self.root.after(QUEUE_POLL_MS, self._poll_queue)

    def _add_device(self, device: dict) -> None:
        key = device[COL_HOSTNAME] or device[COL_IP]
        if key in self.devices_by_host:
            return
        self.devices_by_host[key] = device
        self.tree.insert(
            "",
            tk.END,
            iid=key,
            values=(device[COL_HOSTNAME], device[COL_IP], device[COL_PORT]),
        )
        if self.scanning:
            self.status_var.set(f"Scanning... {len(self.devices_by_host)} found")

    def open_selected(self) -> None:
        selection = self.tree.selection()
        if not selection:
            return
        device = self.devices_by_host.get(selection[0])
        if device is None:
            return
        host = device[COL_HOSTNAME]
        ip = device[COL_IP]
        if host:
            webbrowser.open(f"http://{host}/")
        elif ip:
            webbrowser.open(f"http://{ip}/")


def main() -> None:
    root = tk.Tk()
    FlickerDiscoveryApp(root)
    root.mainloop()


if __name__ == "__main__":
    main()
