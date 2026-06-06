"""
진입점. 2D / 3D 측위 모드를 선택해 실행합니다.

실행:
    python main.py            # 기본 3D
    python main.py --mode 2d  # 2D 평면 측위
    python main.py --mode 3d  # 3D 측위
    python main.py -m 2d

필요 패키지: PyQt5, pyqtgraph, numpy
    pip install PyQt5 pyqtgraph numpy
"""

import sys
import argparse


def main():
    parser = argparse.ArgumentParser(description="UWB Positioning Monitor")
    parser.add_argument("-m", "--mode", choices=["2d", "3d"], default="3d",
                        help="측위 차원 선택 (기본: 3d)")
    args = parser.parse_args()

    # 무거운 GUI import는 인자 파싱 이후로 미룸 (--help가 빠르게 동작)
    from PyQt5.QtWidgets import QApplication

    app = QApplication(sys.argv)

    if args.mode == "2d":
        from visualizer_2d import Visualizer2D
        v = Visualizer2D()
    else:
        from visualizer_3d import Visualizer3D
        v = Visualizer3D()

    v.show()
    sys.exit(app.exec_())


if __name__ == "__main__":
    main()
