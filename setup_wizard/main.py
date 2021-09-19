nfrom kivy.lang import Builder
from kivymd.app import MDApp
from kivy.core.window import Window

KV = '''
<Home@Screen>
<Components@Screen>

ScreenManager:
    Home:
    Components:

<Home>:
    name: "home"
    MDBoxLayout:
        orientation: "vertical"
        padding: "20dp"
        MDLabel:
            text: "Welcome to LCC Setup Wizard"
            font_style: "H4"
            halign: "center"
        MDLabel:
            text: "The setup will guide you through the installation of LCC on your machine, it's recommended to [b]close[/b] all other applications before starting the setup."
            halign: "center"
            markup: True
            theme_text_color: "Secondary"
        BoxLayout:
    MDRaisedButton:
        text: "NEXT"
        pos_hint: {"center_x": 0.9, "center_y": 0.1}
        on_release: root.manager.current = "components"
    MDFlatButton:
        text: "CLOSE"
        pos_hint: {"center_x": 0.73, "center_y": 0.1}
        on_release: quit()
        
        
<Components>:
    name: "components"
    BoxLayout:
        orientation: 'vertical'
        padding: "20dp"
        MDLabel:
            text: "[b]Select the components you want to install[/b]:"
            halign: "center"
            markup: True
            font_style: "H6"
        BoxLayout:
    MDCheckbox:
        pos_hint: {"center_x": 0.25, "center_y": 0.6}
        size_hint: None, None
    MDCheckbox:
        pos_hint: {"center_x": 0.25, "center_y": 0.45} ##### how is the ui? i don't see the ui from here on pycharm  lol, lets run || can u see now? no, only what's written in the terminal
        size_hint: None, None
    MDLabel:
        text: "LCC (with all standard libraries)"
        halign: "center"
        pos_hint: {"center_y": 0.6}
    MDLabel:
        text: "GCC (install if you don't have)"
        halign: "center"
        pos_hint: {"center_y": 0.45}
'''


def restrict_resizing(*args) -> None:
    """ Prevents the window from being resized """
    Window.size = Window.minimum_width, Window.minimum_height


class SetupWizard(MDApp):
    def __init__(self, **kwargs):
        super().__init__(**kwargs)
        self.title = "LCC Setup Wizard"

    def build(self):
        Window.size = 550, 400
        Window.minimum_width, Window.minimum_height = Window.size
        Window.bind(on_resize=restrict_resizing)
        return Builder.load_string(KV)


SetupWizard().run()
