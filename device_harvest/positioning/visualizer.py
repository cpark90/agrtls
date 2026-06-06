"""
하위 호환 shim.
기존에 `from visualizer import UwbVisualizer`를 쓰던 코드를 위해 3D를 재노출합니다.
신규 코드는 visualizer_2d / visualizer_3d 를 직접 쓰세요.
"""

from visualizer_3d import Visualizer3D as UwbVisualizer  # noqa: F401
