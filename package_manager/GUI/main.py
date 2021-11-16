from kivy.lang import Builder
from kivymd.app import MDApp

KV = '''
Screen:
    
'''


class LpmGui(MDApp):
    def __init__(self, **kwargs):
        super().__init__(**kwargs)

    def build(self):
        return Builder.load_string(KV)

    def on_start(self):
        self.theme_cls.theme_style = "Dark"


if __name__ == '__main__':
    LpmGui().run()
