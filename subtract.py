"""A most basic algorithm.

This test code implements the smallest possible run that does something
real. It serves as a "Hello, World" equivalent for running Python code.
"""

def subtract(i: int, j: int) -> int:
    """Subtract 'j' from 'i' and return the difference.

    Args:
        i (int): First input.
        j (int): Second input.

    Returns:
        int: Difference of 'i' and 'j'

    Examples:
        >>> subtract(1, 2)
        -1
    """
    return i - j


def PHLEX_REGISTER_ALGORITHMS(m, config):
    """Register the `subtract` algorithm as a transformation.

    Use the standard Phlex `transform` registration to insert a node
    in the execution graph that receives two inputs and produces their
    sum as an ouput. The labels of inputs and outputs are taken from
    the configuration.

    Args:
        m (internal): Phlex registrar representation.
        config (internal): Phlex configuration representation.

    Returns:
        None
    """
    m.transform(subtract,
                input_family = config["input"],
                output_product_suffixes = config["output"])
