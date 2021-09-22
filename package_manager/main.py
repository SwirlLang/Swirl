""" Source code for the package manager GUI"""

import os
from kivy.core.window import Window
from kivy.lang import Builder
from kivymd.app import MDApp

KV = '''
Screen:
    BoxLayout:
        MDCard:
            orientation: 'vertical'
            size_hint_x: 0.37
            OneLineIconListItem:
                text: "Built-ins"
                theme_text_color: "Custom"
                text_color: 0, 0, 0, 1
                IconLeftWidget:
                    icon: "folder"
                    theme_text_color: "Custom"
                    text_color: 0, 0, 0, 1         
            OneLineIconListItem:
                text: "Installed"
                theme_text_color: "Custom"
                text_color: 0, 0, 0, 1
                IconLeftWidget:
                    icon: "download"
                    theme_text_color: "Custom"
                    text_color: 0, 0, 0, 1  
            OneLineIconListItem:
                text: "Github"
                theme_text_color: "Custom"
                text_color: 0, 0, 0, 1
                IconLeftWidget:
                    icon: "github"
                    theme_text_color: "Custom"
                    text_color: 0, 0, 0, 1  
            BoxLayout:         
        BoxLayout:
            orientation: "vertical"
'''


class PackageManager(MDApp):
    def __init__(self, **kwargs):
        super().__init__(**kwargs)
        self.title = "LambdaCode Package Manager(GUI)"

    def build(self):
        return Builder.load_string(KV)


if __name__ == '__main__':
    PackageManager().run()
