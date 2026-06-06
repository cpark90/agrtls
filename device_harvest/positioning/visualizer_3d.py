"""
3D 시각화.
맵을 X-Y(평면도) + X-Z(측면도) 두 뷰로 표시합니다.
"""

import numpy as np
import pyqtgraph as pg

import config
from visualizer_base import BaseVisualizer


class Visualizer3D(BaseVisualizer):
    def __init__(self):
        super().__init__(dim=3,
            title="UWB 3D Positioning (Multilateration + Kalman)")

    def _build_map_ui(self):
        # 행2: X-Y 평면도 / X-Z 측면도
        self.pmap_xy = self.win.addPlot(row=1, col=0, title="Position Map (Top View X-Y)")
        self.pmap_xy.setLabel('left', 'Y', units='m')
        self.pmap_xy.setLabel('bottom', 'X', units='m')
        self.pmap_xy.showGrid(x=True, y=True)
        self.pmap_xy.setAspectLocked(True)

        self.pmap_xz = self.win.addPlot(row=1, col=1, title="Position Map (Side View X-Z)")
        self.pmap_xz.setLabel('left', 'Z', units='m')
        self.pmap_xz.setLabel('bottom', 'X', units='m')
        self.pmap_xz.showGrid(x=True, y=True)
        self.pmap_xz.setAspectLocked(True)

        # 앵커 고정점 + 라벨
        axy, axz = [], []
        for dev_id, (x, y, z) in self.anchors.items():
            axy.append((x, y)); axz.append((x, z))
            color = self.color_map[dev_id]
            for plot, (a, b) in ((self.pmap_xy, (x, y)), (self.pmap_xz, (x, z))):
                txt = pg.TextItem(dev_id, color=color, anchor=(0.5, 1.2))
                txt.setPos(a, b)
                plot.addItem(txt)
        if axy:
            self.pmap_xy.plot([p[0] for p in axy], [p[1] for p in axy],
                pen=None, symbol='s', symbolBrush=(100, 100, 255), symbolSize=12, name="Anchors")
            self.pmap_xz.plot([p[0] for p in axz], [p[1] for p in axz],
                pen=None, symbol='s', symbolBrush=(100, 100, 255), symbolSize=12)

        # 축 자동 범위 (X, Y, Z)
        b = self._anchor_axis_bounds()
        if b:
            self.pmap_xy.setXRange(*b[0]); self.pmap_xy.setYRange(*b[1])
            self.pmap_xz.setXRange(*b[0]); self.pmap_xz.setYRange(*b[2])

        # 태그 현재 위치 / 궤적
        self.tag_xy = self.pmap_xy.plot(pen=None, symbol='o',
            symbolBrush=(255, 80, 80), symbolSize=14)
        self.trail_xy = self.pmap_xy.plot(pen=pg.mkPen((255, 80, 80, 120), width=2))
        self.tag_xz = self.pmap_xz.plot(pen=None, symbol='o',
            symbolBrush=(255, 80, 80), symbolSize=14)
        self.trail_xz = self.pmap_xz.plot(pen=pg.mkPen((255, 80, 80, 120), width=2))

    def _update_map(self, cutoff):
        if not self.pos_hist:
            return
        xs = [h[1] for h in self.pos_hist]
        ys = [h[2] for h in self.pos_hist]
        zs = [h[3] for h in self.pos_hist]
        self.trail_xy.setData(xs, ys)
        self.trail_xz.setData(xs, zs)
        self.tag_xy.setData([xs[-1]], [ys[-1]])
        self.tag_xz.setData([xs[-1]], [zs[-1]])
