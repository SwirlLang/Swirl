import sys

from PyQt5.QtCore import Qt
from PyQt5.QtWidgets import QApplication, QLabel, QMainWindow
from PyQt5.QtWidgets import QToolBar
from PyQt5.QtWidgets import QMenu


class Window(QMainWindow):
    def __init__(self, parent=None):
        super().__init__(parent)
        self.setWindowTitle("Epsilon")
        self.resize(400, 200)
        # self.setStyleSheet("background-color: black")
        self.centralWidget = QLabel("Hello, World")
        self.centralWidget.setAlignment(Qt.AlignHCenter | Qt.AlignVCenter)
        self.setCentralWidget(self.centralWidget)
        self.create_context_menu()

        self.create_menu_panel()
        self.showMaximized()
        self.create_toolbar()

    def create_menu_panel(self):
        menu_bar = self.menuBar()

        fileMenu = QMenu("&File", self)
        fileMenuList = [
            "New project...",
            "Open",
            "Save as",
            "Rename project",
            "Settings",
            "Reload all from disk",
            "Exit"
        ]
        for file_manu in fileMenuList:
            fileMenu.addAction(file_manu)
        editMenu = QMenu("&Edit", self)
        navMenu = QMenu("&Navigate", self)
        refactorMenu = QMenu("&Refactor", self)
        vcsMenu = QMenu("&VCS", self)
        helpMenu = QMenu("&Help", self)
        menuList = [
            fileMenu, editMenu, navMenu, refactorMenu,
            vcsMenu, helpMenu
        ]

        for menu in menuList:
            menu_bar.addMenu(menu)

    def create_toolbar(self):
        file = self.addToolBar("File")
        edit = QToolBar("Edit", self)
        self.addToolBar(edit)
        help = QToolBar("Help", self)
        self.addToolBar(Qt.LeftToolBarArea, help)

    # def create_context_menu(self):
    #     self.centralWidget.setContextMenuPolicy(Qt.ActionsContextMenu)
    #     self.centralWidget.addAction(Q\77)
    #     self.centralWidget.addAction("Run")
    #     self.centralWidget.addAction()
    #     self.centralWidget.addAction(self.copyAction)
    #     self.centralWidget.addAction(self.pasteAction)
    #     self.centralWidget.addAction(self.cutAction)


if __name__ == "__main__":
    app = QApplication(sys.argv)
    win = Window()
    win.show()
    sys.exit(app.exec_())
