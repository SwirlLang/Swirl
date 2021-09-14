""" Class defining all naming conventions and the formatter for Lambda-code (still in development)"""


class Conventions:
  def apply_all(self, snippet: str) -> None:
    """ apply all conventions to the snippet
    :param snippet: Any content yo apply the convention on    
    """
    self.apply_camel_case(snippet)
    self.apply_pascal_case(snippet)
    self.apply_snake_case(snippet)
  
  def apply_camel_case(self, snippet: str) -> None:
    """ apply camel case convention to the given snippet of code
    :param snippet: Any content yo apply the convention on
    
    Description: in Lambda-code camel case convention should be applied to all functions
    """
    
  def apply_snake_case(self, snippet: str) -> None:
    """ apply snake case convention to the given snippet 
    :param snippet: Any content yo apply the convention on
    
    Description: in Lambda-code snake case conventions should be applied to all class variables (top-level vars can be written in any form)
    """
  def apply_pascal_case(self, snippet: str) -> None:
    """ apply camel case convention to the given snippet of code
    :param snippet: Any content yo apply the convention on
    
    Description: in Lambda-code camel case convention should be applied to all classes
    """
