import controlP5.*;
import java.awt.datatransfer.*;
import java.awt.Toolkit;
import java.util.ArrayList; // ArrayList를 사용하므로 명시적으로 import
import java.time.LocalDateTime; // 날짜/시간을 위해 추가
import java.time.format.DateTimeFormatter; // 날짜/시간 포맷을 위해 추가

ControlP5 cp5;
Textarea bufferView1;
Textarea bufferView2;
Textfield inputText;
RadioButton modeSelector;
color[][] ledMatrix1;
color[][] ledMatrix2;
color[][] grandMatrix; // cols * 2 크기로 두 매트릭스를 합친 그림을 그릴 PGraphics용
boolean useWebSafe = true;
int cols = 28, rows = 35;

void setup() {
  size(1200, 840); // 윈도우 크기 설정
  cp5 = new ControlP5(this); // ControlP5 초기화

  // 1. LED 매트릭스 배열 초기화 (가장 먼저)
  ledMatrix1 = new color[cols][rows];
  ledMatrix2 = new color[cols][rows];
  grandMatrix = new color[cols * 2][rows];

  // 도형 버튼 생성
  String[] shapes = {"Square", "Circle", "Triangle", "Watermelon", "Arrow", "Boat", "Taegukgi", "Christmas Tree", "Luxtep", "Gauge1", "Date", "Clear"};
  for (int i = 0; i < shapes.length; i++) {
    cp5.addButton(shapes[i])
      .setPosition(850 + (i % 4) * 80, 100 + (i / 4) * 40) // 버튼 위치 설정
      .setSize(60, 25); // 버튼 크기 설정
  }

  // 2. 모든 UI 요소 초기화 (Textarea 포함)
  // 텍스트 입력 필드
  inputText = cp5.addTextfield("inputText")
    .setPosition(840, 420)
    .setSize(150, 120) // 높이를 2배로 증가 (60 -> 120)
    .setFont(createFont("Tahoma", 12)) // 폰트 설정
    .setColor(color(0))
    .setColorBackground(color(255))
    .setColorForeground(color(0))
    .setText(""); // 초기값을 빈칸으로 설정

  // 줄바꿈 버튼 추가
  cp5.addButton("Line Break")
    .setPosition(840 + 150 + 10, 420) // inputText 우측에 배치
    .setSize(60, 30);

  // Paste 버튼 추가
  cp5.addButton("Paste")
    .setPosition(840 + 150 + 10, 420 + 30 + 5) // Line Break 버튼 아래에 배치
    .setSize(60, 30);

  // "Show" 버튼 (텍스트 입력 후 그리기)
  cp5.addButton("Show")
    .setPosition(840 + 150 + 10, 420 + 2 * (30 + 5)) // Paste 버튼 아래에 배치
    .setSize(60, 30);

  // "Clear Text" 버튼 추가
  cp5.addButton("Clear Text")
    .setPosition(840 + 150 + 10, 420 + 3 * (30 + 5)) // Show 버튼 아래에 배치
    .setSize(60, 30);

  // 색상 모드 선택 라디오 버튼 (이제 bufferView1/2가 초기화됨)
  modeSelector = cp5.addRadioButton("mode")
    .setPosition(20, 700)
    .setSize(20, 20)
    .addItem("Web Safety Color 216", 0) // 웹 안전 색상 모드 (기본값)
    .addItem("RGB Mode", 1)             // RGB 모드
    .setFont(createFont("Tahoma", 12)) // 폰트 설정
    .setColorBackground(color(155))
    .setColorForeground(color(15)); // activate(0)는 나중에 호출

  // 라디오 버튼 아이템의 캡션 라벨 색상 설정
  for (Toggle t : modeSelector.getItems()) {
    t.getCaptionLabel().setColor(color(200));
  }

  // OSD 버퍼 뷰 1 (왼쪽 매트릭스)
  bufferView1 = cp5.addTextarea("osd_buf_L")
    .setPosition(20, 600)
    .setSize(300, 80)
    .setFont(createFont("Tahoma", 8)) // 폰트 설정
    .setLineHeight(14)
    .setColor(color(0))
    .setColorBackground(color(255))
    .setColorForeground(color(0));

  // "copy board#1" 버튼
  cp5.addButton("copy board#1")
    .setPosition(310, 600+3)
    .setSize(80, 30);

  // OSD 버퍼 뷰 2 (오른쪽 매트릭스)
  bufferView2 = cp5.addTextarea("osd_buf_R")
    .setPosition(420, 600)
    .setSize(300, 80)
    .setFont(createFont("Tahoma", 8)) // 폰트 설정
    .setLineHeight(14)
    .setColor(color(0))
    .setColorBackground(color(255))
    .setColorForeground(color(0));

  // "copy board#0" 버튼
  cp5.addButton("copy board#0")
    .setPosition(710, 600+3)
    .setSize(80, 30);

  // 3. 모든 UI 요소가 초기화된 후, 라디오 버튼 활성화 및 매트릭스 초기화
  modeSelector.activate(0); // 이제 bufferView1/2가 안전하게 초기화되었으므로 호출 가능
  clearMatrix(); // 이 안에서 updateBuffer()가 호출됨
}

void draw() {
  background(0); // 배경을 검은색으로 설정

  fill(0, 255, 255);
  textAlign(LEFT, TOP);
  textSize(16);
  text("OSD simulation for board 1", 20, 40);
  text("OSD simulation for board 0", 410, 40);

  text("OSD Buffer for board 1", 20, 580);
  text("OSD Buffer for board 0", 420, 580);

  
  int start_y = 70; // 매트릭스 시작 Y 좌표
  int cellSize = 13; // 각 LED 셀의 크기
  int cell_distance = 2; // 셀 간의 간격

  // 첫 번째 LED 매트릭스 그리기
  for (int y = 0; y < rows; y++) {
    for (int x = 0; x < cols; x++) {
      fill(ledMatrix1[x][y]); // 해당 픽셀의 색상으로 채우기
      rect(20 + x * cellSize, start_y + y * cellSize, cellSize - cell_distance, cellSize - cell_distance); // 사각형 그리기
    }
  }
  // 두 번째 LED 매트릭스 그리기 (첫 번째 매트릭스 옆에)
  for (int y = 0; y < rows; y++) {
    for (int x = 0; x < cols; x++) {
      fill(ledMatrix2[x][y]); // 해당 픽셀의 색상으로 채우기
      // X 좌표를 조정하여 첫 번째 매트릭스 옆에 오도록 하고, 두 매트릭스 사이에 20픽셀 간격 추가
      rect(20 + (cols + x) * cellSize + 20, start_y + y * cellSize, cellSize - cell_distance, cellSize - cell_distance);
    }
  }
}

// ControlP5 이벤트 핸들러
void controlEvent(ControlEvent e) {
  // 라디오 버튼 이벤트 처리
  if (e.isFrom(modeSelector)) {
    useWebSafe = (int)e.getValue() == 0; // 선택된 모드에 따라 useWebSafe 값 설정
    updateBuffer(); // 버퍼 업데이트
  } else {
    // 다른 버튼 이벤트 처리
    String name = e.getName();
    if (name.equals("copy board#1")) {
      // bufferView1의 텍스트를 클립보드에 복사
      StringSelection stringSelection = new StringSelection(bufferView1.getText());
      Toolkit.getDefaultToolkit().getSystemClipboard().setContents(stringSelection, null);
    } else if (name.equals("copy board#0")) {
      // bufferView2의 텍스트를 클립보드에 복사
      StringSelection stringSelection = new StringSelection(bufferView2.getText());
      Toolkit.getDefaultToolkit().getSystemClipboard().setContents(stringSelection, null);
    } else if (name.equals("Show")) {
      // 텍스트 입력 필드의 텍스트를 LED 매트릭스에 그리기
      drawText(inputText.getText());
    } else if (name.equals("Line Break")) { // 줄바꿈 버튼 클릭 시
      inputText.setText(inputText.getText() + "\n"); // 텍스트 필드에 줄바꿈 문자 추가
    } else if (name.equals("Paste")) { // Paste 버튼 클릭 시
      // 클립보드에서 텍스트 가져와 inputText에 붙여넣기
      Clipboard clipboard = Toolkit.getDefaultToolkit().getSystemClipboard();
      Transferable contents = clipboard.getContents(null);
      if (contents != null && contents.isDataFlavorSupported(DataFlavor.stringFlavor)) {
        try {
          String clipboardText = (String) contents.getTransferData(DataFlavor.stringFlavor);
          inputText.setText(inputText.getText() + clipboardText);
        } catch (UnsupportedFlavorException | java.io.IOException ex) {
          println("Error pasting from clipboard: " + ex.getMessage());
        }
      }
    } else if (name.equals("Clear Text")) { // "Clear Text" 버튼 클릭 시
      inputText.setText(""); // inputText 필드 비우기
    } else if (name.equals("Clear")) { // "Clear" 버튼 (매트릭스 전체 초기화)
      clearMatrix();
    } else {
      // 도형 버튼 클릭 시 해당 도형 그리기
      drawGrandPicture(name);
    }
  }
}

// LED 매트릭스를 모두 검은색으로 초기화
void clearMatrix() {
  stroke(22); // 테두리 색상 (매트릭스 픽셀 테두리)
  for (int y = 0; y < rows; y++) {
    for (int x = 0; x < cols; x++) {
      ledMatrix1[x][y] = color(0); // 검은색 (OFF)
      ledMatrix2[x][y] = color(0); // 검은색 (OFF)
    }
  }
  // grandMatrix도 초기화 (그림 그리기 전에 항상 초기화되므로 여기서 필수는 아님)
  for (int y = 0; y < rows; y++) {
    for (int x = 0; x < cols * 2; x++) {
      grandMatrix[x][y] = color(0);
    }
  }
  updateBuffer(); // 매트릭스 클리어 후 버퍼 업데이트
}

// 텍스트를 LED 매트릭스에 그리는 함수
void drawText(String txt) {
  drawGrandPicture(txt); // drawGrandPicture 함수를 사용하여 텍스트 렌더링
}

// grandMatrix의 내용을 ledMatrix1과 ledMatrix2로 분할
void dividePicture() {
  for (int y = 0; y < rows; y++) {
    for (int x = 0; x < cols; x++) {
      ledMatrix1[x][y] = grandMatrix[x][y]; // grandMatrix의 왼쪽 절반을 ledMatrix1에
      ledMatrix2[x][y] = grandMatrix[x + cols][y]; // grandMatrix의 오른쪽 절반을 ledMatrix2에
    }
  }
}

// 현재 ledMatrix1과 ledMatrix2의 내용을 기반으로 버퍼 문자열을 업데이트
void updateBuffer() {
  ArrayList<String> list1 = new ArrayList<String>();
  ArrayList<String> list2 = new ArrayList<String>();

  // Y 좌표를 역순으로 반복하여 버퍼에 채웁니다. (rows - 1 부터 0까지)
  for (int y = rows - 1; y >= 0; y--) { 
    for (int x = 0; x < cols; x++) {
      color c1 = ledMatrix1[x][y];
      color c2 = ledMatrix2[x][y];
      list1.addAll(colorToBuffer(c1)); // 색상을 버퍼 형식으로 변환하여 추가
      list2.addAll(colorToBuffer(c2));
    }
  }
  // 버퍼 문자열을 Textarea에 설정 (C/C++ 스타일 배열 선언 포함)
  //bufferView1.setText("int [] osd_buf_L={" + join(list1.toArray(new String[0]), ", ") + "};");
  //bufferView2.setText("int [] osd_buf_R={" + join(list2.toArray(new String[0]), ", ") + "};");
  bufferView1.setText(join(list1.toArray(new String[0]), ", "));
  bufferView2.setText(join(list2.toArray(new String[0]), ", "));
}

// Processing color 값을 버퍼에 저장할 String 형식으로 변환
ArrayList<String> colorToBuffer(color c) {
  ArrayList<String> out = new ArrayList<String>();
  int r = int(red(c));
  int g = int(green(c));
  int b = int(blue(c));
  if (useWebSafe) {
    // 웹 안전 색상 인덱스 계산 (6x6x6 큐브)
    int rs = round(r / 51.0) * 51;
    int gs = round(g / 51.0) * 51;
    int bs = round(b / 51.0) * 51;
    int idx = (rs / 51) * 36 + (gs / 51) * 6 + (bs / 51);
    out.add("" + idx);
  } else {
    // RGB 모드
    out.add("" + r);
    out.add("" + g);
    out.add("" + b);
  }
  return out;
}

// grandMatrix에 그림을 그리거나 텍스트를 렌더링하고, 이를 두 개의 LED 매트릭스로 나눕니다.
void drawGrandPicture(String name) {
  // grandMatrix를 항상 검은색으로 초기화
  for (int y = 0; y < rows; y++) {
    for (int x = 0; x < cols * 2; x++) {
      grandMatrix[x][y] = color(0);
    }
  }

  PGraphics pg = createGraphics(cols * 2, rows); // 두 매트릭스 너비에 해당하는 PGraphics 생성
  pg.beginDraw();
  pg.background(0); // PGraphics 배경을 검은색으로 설정 (투명하지 않음)
  pg.textAlign(CENTER, CENTER); // 텍스트 정렬을 중앙으로 설정
  pg.textSize(12); // 텍스트 크기 조정
  pg.fill(color(255, 255, 0)); // 그림/텍스트의 기본 색상 (노란색)

  int cx = pg.width / 2; // PGraphics의 중앙 X 좌표
  int cy = pg.height / 2; // PGraphics의 중앙 Y 좌표

  // 도형 그리기 로직 (PGraphics에 그림)
  if (name.equals("Square")) {
    pg.noStroke();
    // 사각형 그라데이션 (왼쪽에서 오른쪽으로: 초록 -> 노랑 -> 주황 -> 빨강 -> 파랑)
    for (int xOffset = -15; xOffset <= 15; xOffset++) {
        float gradientPos = map(xOffset, -15, 15, 0, 1); // Normalized position 0 to 1
        color c;
        if (gradientPos < 0.25) { // Green to Yellow
            c = lerpColor(color(0, 255, 0), color(255, 255, 0), map(gradientPos, 0, 0.25, 0, 1));
        } else if (gradientPos < 0.5) { // Yellow to Orange
            c = lerpColor(color(255, 255, 0), color(255, 165, 0), map(gradientPos, 0.25, 0.5, 0, 1));
        } else if (gradientPos < 0.75) { // Orange to Red
            c = lerpColor(color(255, 165, 0), color(255, 0, 0), map(gradientPos, 0.5, 0.75, 0, 1));
        } else { // Red to Blue (추가된 파랑)
            c = lerpColor(color(255, 0, 0), color(0, 0, 255), map(gradientPos, 0.75, 1, 0, 1));
        }
        pg.fill(c);
        pg.rect(cx + xOffset, cy - 15, 1, 30); // 얇은 사각형으로 그라데이션
    }
  } else if (name.equals("Circle")) {
    pg.noStroke();
    // 원형 그라데이션 (구체처럼 명암 표현: 중앙에서 바깥으로, 밝은 색 -> 어두운 색)
    int radius = 15; // Max radius
    for (int y = -radius; y <= radius; y++) {
      for (int x = -radius; x <= radius; x++) {
        float distSq = x*x + y*y;
        if (distSq <= radius*radius) {
          float distToCenter = sqrt(distSq);
          // 색상 변화를 위한 정규화된 거리 (중앙 0, 가장자리 1)
          float normalizedDist = map(distToCenter, 0, radius, 0, 1);

          // 색상 그라데이션 (밝은 초록 -> 노랑 -> 주황 -> 빨강 -> 파랑)
          color c;
          if (normalizedDist < 0.25) { // Green to Yellow
              c = lerpColor(color(0, 255, 0), color(255, 255, 0), map(normalizedDist, 0, 0.25, 0, 1));
          } else if (normalizedDist < 0.5) { // Yellow to Orange
              c = lerpColor(color(255, 255, 0), color(255, 165, 0), map(normalizedDist, 0.25, 0.5, 0, 1));
          } else if (normalizedDist < 0.75) { // Orange to Red
              c = lerpColor(color(255, 165, 0), color(255, 0, 0), map(normalizedDist, 0.5, 0.75, 0, 1));
          } else { // Red to Blue
              c = lerpColor(color(255, 0, 0), color(0, 0, 255), map(normalizedDist, 0.75, 1, 0, 1));
          }
          pg.set(cx + x, cy + y, c);
        }
      }
    }
  } else if (name.equals("Car")) {
    // 자동차 형태 변경 및 그라데이션 (이미지 참고)
    pg.noStroke();
    // 차체 (빨간색 그라데이션)
    int carBodyWidth = 30;
    int carBodyHeight = 10;
    for (int yOffset = 0; yOffset < carBodyHeight; yOffset++) {
        float grad = map(yOffset, 0, carBodyHeight, 0, 1);
        pg.fill(lerpColor(color(255, 50, 50), color(150, 0, 0), grad));
        pg.rect(cx - carBodyWidth/2, cy + yOffset - carBodyHeight/2 + 5, carBodyWidth, 1);
    }
    // 지붕 (파란색 그라데이션)
    int roofWidth = 18;
    int roofHeight = 7;
    for (int yOffset = 0; yOffset < roofHeight; yOffset++) {
        float grad = map(yOffset, 0, roofHeight, 0, 1);
        pg.fill(lerpColor(color(50, 50, 255), color(0, 0, 150), grad));
        pg.rect(cx - roofWidth/2, cy + yOffset - roofHeight/2 - 8, roofWidth, 1);
    }
    // 바퀴 (검은색)
    pg.fill(0);
    pg.ellipse(cx - 10, cy + 8, 8, 8);
    pg.ellipse(cx + 10, cy + 8, 8, 8);
  } else if (name.equals("Triangle")) {
    pg.noStroke();
    // 삼각형 그라데이션 (아래에서 위로)
    for (int y = -15; y <= 15; y++) {
      float currentWidth = map(y, -15, 15, 0, 30); // y에 따라 너비 변화
      int startX = (int)(cx - currentWidth / 2);
      int endX = (int)(cx + currentWidth / 2);
      for (int x = startX; x <= endX; x++) {
        float gradient = map(y, -15, 15, 0, 1); // y 위치에 따른 그라데이션
        pg.set(x, cy + y, lerpColor(color(0, 255, 0), color(0, 100, 0), gradient));
      }
    }
  } else if (name.equals("Watermelon")) { // 수박
    pg.noStroke();
    int watermelonWidth = 40; // 수박 전체 너비 (PGraphics 픽셀 기준)
    int watermelonHeight = 30; // 수박 전체 높이 (PGraphics 픽셀 기준)

    // 수박 몸통 그리기 (PGraphics 픽셀 단위로)
    for (int y = 0; y < watermelonHeight; y++) {
      for (int x = 0; x < watermelonWidth; x++) {
        float normalizedX = map(x, 0, watermelonWidth, -1, 1);
        float normalizedY = map(y, 0, watermelonHeight, -1, 1);

        // 타원형 수박 모양 내부에 있는지 확인
        if (pow(normalizedX, 2) + pow(normalizedY, 2) < 1) {
          color pixelColor;
          float distFromCenter = sqrt(pow(normalizedX, 2) + pow(normalizedY, 2));

          if (distFromCenter > 0.85) { // 가장 바깥 껍질 (진한 초록)
            pixelColor = lerpColor(color(0, 100, 0), color(0, 50, 0), map(distFromCenter, 0.85, 1, 0, 1));
            // 줄무늬 추가 (간단한 수직 줄무늬)
            if (abs(normalizedX * 10) % 2 < 1) { // 짝수/홀수 줄무늬
                pixelColor = lerpColor(pixelColor, color(0, 20, 0), 0.5); // 줄무늬를 더 어둡게
            }
          } else if (distFromCenter > 0.7) { // 중간 껍질 (연한 초록/흰색)
            pixelColor = lerpColor(color(150, 255, 150), color(200, 255, 200), map(distFromCenter, 0.7, 0.85, 0, 1));
          } else { // 과육 (빨강/분홍 그라데이션)
            pixelColor = lerpColor(color(255, 50, 50), color(255, 150, 150), map(distFromCenter, 0, 0.7, 0, 1));
          }
          pg.set(cx + x - watermelonWidth/2, cy + y - watermelonHeight/2, pixelColor);
        }
      }
    }

    // 씨앗 추가 (간단한 검은색 타원)
    pg.fill(0);
    pg.ellipse(cx - 5, cy - 5, 3, 3);
    pg.ellipse(cx + 5, cy - 3, 3, 3);
    pg.ellipse(cx, cy + 5, 3, 3);
    pg.ellipse(cx - 8, cy + 2, 3, 3);
    pg.ellipse(cx + 8, cy + 2, 3, 3);

  } else if (name.equals("Arrow")) {
    pg.pushMatrix();
    pg.translate(cx, cy); // PGraphics 원점을 중앙으로 이동

    // 대각선 방향으로 회전 (45도)
    pg.rotate(PI / 4); // 시계 방향 45도 회전

    // 화살표 몸통 (직사각형, 그라데이션)
    int bodyWidth = 8;
    int bodyHeight = 30; // 몸통 길이 증가
    pg.noStroke();
    for (int y = -bodyHeight / 2; y < bodyHeight / 2; y++) {
        float grad = map(y, -bodyHeight / 2, bodyHeight / 2, 0, 1);
        pg.fill(lerpColor(color(255, 255, 0), color(255, 100, 0), grad)); // 노랑 -> 주황 그라데이션
        pg.rect(-bodyWidth / 2, y, bodyWidth, 1); // 얇은 가로선으로 그라데이션 표현
    }

    // 화살표 머리 (삼각형)
    pg.fill(255, 100, 0); // 주황색 머리
    pg.triangle(0, -bodyHeight / 2 - 5, // 뾰족한 끝
                -bodyWidth / 2 - 5, -bodyHeight / 2 + 5, // 왼쪽 아래 점
                bodyWidth / 2 + 5, -bodyHeight / 2 + 5); // 오른쪽 아래 점

    // 화살표 꼬리 (갈색)
    int tailWidth = 6;
    int tailHeight = 15; // 꼬리 길이
    pg.fill(139, 69, 19); // 갈색
    pg.rect(-tailWidth / 2, bodyHeight / 2, tailWidth, tailHeight);

    pg.popMatrix(); // 원래 변환 상태로 복원

  } else if (name.equals("Boat")) {
    pg.noStroke();
    // 배 몸통 그라데이션 (갈색에서 진한 갈색)
    pg.fill(lerpColor(color(200, 100, 0), color(100, 50, 0), 0.3));
    pg.triangle(cx - 20, cy + 10, cx + 20, cy + 10, cx, cy + 20); // 배 바닥

    // 바다 표현 (배 아래에 파란색 그라데이션)
    int waterHeight = 10;
    for (int yOffset = 0; yOffset < waterHeight; yOffset++) {
        float grad = map(yOffset, 0, waterHeight, 0, 1);
        pg.fill(lerpColor(color(0, 100, 200), color(0, 0, 100), grad)); // 밝은 파랑 -> 어두운 파랑
        pg.rect(cx - 25, cy + 15 + yOffset, 50, 1); // 배 아래에 넓게
    }
    
    pg.fill(255); // 흰색 돛 (그라데이션 없음)
    pg.rect(cx - 1, cy - 10, 2, 20); // 돛대
    pg.triangle(cx, cy - 10, cx + 15, cy, cx, cy + 10); // 돛
  } else if (name.equals("Taegukgi")) { // 태극기 (이미지 참고하여 다시 그리기, 4괘 제외)
    pg.noStroke();
    float taegukSize = 25; // 태극 문양의 지름 (더 작게 조절)
    float taegukRadius = taegukSize / 2;

    pg.pushMatrix(); // 현재 변환 상태 저장
    pg.translate(cx, cy); // 원점을 태극 문양 중앙으로 이동
    // pg.scale(-1, 1); // 좌우 반전 제거

    // PGraphics를 사용하여 태극 문양을 픽셀 단위로 정확하게 그리기
    for (int y = 0; y < pg.height; y++) {
      for (int x = 0; x < pg.width; x++) {
        float dx = x - cx;
        float dy = y - cy;
        float distSq = dx*dx + dy*dy;

        if (distSq < taegukRadius * taegukRadius) { // 큰 원 내부
          color pixelColor = color(0);
          // 태극 문양 색상 (그라데이션 없음, 고정색)
          if (dy <= 0) { // 위쪽 반원 (빨강)
            pixelColor = color(238, 0, 51);
          } else { // 아래쪽 반원 (파랑)
            pixelColor = color(0, 0, 153);
          }

          // 작은 원들 (빨강 위에 파랑, 파랑 위에 빨강)
          float distSq1 = pow(dx + taegukRadius / 2, 2) + pow(dy, 2);
          float distSq2 = pow(dx - taegukRadius / 2, 2) + pow(dy, 2);

          if (distSq1 < pow(taegukRadius / 2, 2)) { // 왼쪽 작은 원
            pixelColor = color(0, 0, 153); // 파랑
          }
          if (distSq2 < pow(taegukRadius / 2, 2)) { // 오른쪽 작은 원
            pixelColor = color(238, 0, 51); // 빨강
          }
          pg.set(x, y, pixelColor);
        }
      }
    }
    pg.popMatrix(); // 변환 상태 복원
    // 4괘는 그리지 않음
  } else if (name.equals("Christmas Tree")) { // 크리스마스 트리 그리기 (상하 반전, 약간 작게)
    pg.pushMatrix();
    pg.translate(cx, cy); // 원점을 중앙으로 이동
    pg.scale(1, -1); // 상하 반전
    pg.translate(0, -cy); // 원점 다시 조정 (그림이 화면 상단으로 가지 않도록)

    pg.noStroke();

    // 나무 몸통 (그라데이션: 진한 초록 -> 밝은 초록)
    int treeBaseWidth = 25; // 약간 작게
    int treeHeight = 35; // 약간 작게
    for (int y = 0; y < treeHeight; y++) {
      float currentWidth = map(y, 0, treeHeight, treeBaseWidth, 5); // 위로 갈수록 좁아짐
      float grad = map(y, 0, treeHeight, 0, 1);
      pg.fill(lerpColor(color(0, 100, 0), color(0, 200, 0), grad)); // 진한 초록 -> 밝은 초록
      pg.triangle(0, -treeHeight / 2 + y, // cx 대신 0 사용 (translate 때문에)
                  -currentWidth / 2, -treeHeight / 2 + y + 5,
                  currentWidth / 2, -treeHeight / 2 + y + 5);
    }

    // 나무 기둥 (갈색)
    pg.fill(139, 69, 19);
    pg.rect(-5, treeHeight / 2 - 10, 10, 15); // cx 대신 0 사용

    // 장식 (작은 원들, 다양한 색상)
    pg.fill(255, 255, 0); // 노란색 별
    pg.ellipse(0, -treeHeight / 2 - 5, 8, 8); // cx 대신 0 사용

    pg.fill(255, 0, 0); // 빨간색 장식
    pg.ellipse(-8, -10, 5, 5);
    pg.ellipse(10, 5, 5, 5);

    pg.fill(0, 0, 255); // 파란색 장식
    pg.ellipse(5, -15, 5, 5);
    pg.ellipse(-10, 10, 5, 5);

    pg.fill(255, 165, 0); // 주황색 장식
    pg.ellipse(0, 0, 5, 5);

    pg.popMatrix(); // 변환 상태 복원

  } else if (name.equals("Luxtep")) { // Luxtep 로고 그리기 (이미지 참고)
    pg.noStroke();
    
    // 로고 배경색 (파란색)
    pg.fill(0, 0, 153); // 진한 파랑
    pg.rect(cx - 40, cy - 30, 80, 60); // 로고 영역

    // 이미지에 맞춰 흰색 원 그리기
    pg.fill(255); // 흰색
    int dotRadius = 2; // 원의 반지름
    int row1Y = cy - 20;
    int row2Y = cy - 10;
    
    // 첫 번째 줄 원들
    pg.ellipse(cx - 18, row1Y, dotRadius * 2, dotRadius * 2); // 첫 번째 원
    pg.ellipse(cx - 12, row1Y, dotRadius * 2, dotRadius * 2);
    pg.ellipse(cx - 6, row1Y, dotRadius * 2, dotRadius * 2);
    pg.ellipse(cx, row1Y, dotRadius * 2, dotRadius * 2);
    pg.ellipse(cx + 6, row1Y, dotRadius * 2, dotRadius * 2);
    pg.ellipse(cx + 12, row1Y, dotRadius * 2, dotRadius * 2);
    pg.ellipse(cx + 18, row1Y, dotRadius * 2, dotRadius * 2);

    // 두 번째 줄 원들
    pg.ellipse(cx - 15, row2Y, dotRadius * 2, dotRadius * 2); // 첫 번째 원
    pg.ellipse(cx - 9, row2Y, dotRadius * 2, dotRadius * 2);
    pg.ellipse(cx - 3, row2Y, dotRadius * 2, dotRadius * 2);
    pg.ellipse(cx + 3, row2Y, dotRadius * 2, dotRadius * 2);
    pg.ellipse(cx + 9, row2Y, dotRadius * 2, dotRadius * 2);
    pg.ellipse(cx + 15, row2Y, dotRadius * 2, dotRadius * 2);


    // "LUXTEP." 텍스트
    pg.fill(255); // 흰색 텍스트
    pg.textSize(10); // 텍스트 크기
    pg.text("LUXTEP.", cx, cy + 10); // Y 위치 조정
    pg.textSize(6); // 작은 텍스트
    pg.text("Healthcare Co.,Ltd.", cx, cy + 20); // Y 위치 조정

  } else if (name.equals("Gauge1")) { // 게이지 그리기 (이미지 참고)
    pg.noStroke();
    int barWidth = 4;
    int maxBarHeight = 30;
    int numBars = 15; // 이미지에 15개 바가 있음

    for (int i = 0; i < numBars; i++) {
      float barHeight = map(i, 0, numBars - 1, maxBarHeight / numBars, maxBarHeight); // 바 높이 증가
      float barX = cx - (numBars * (barWidth + 1)) / 2 + i * (barWidth + 1); // 중앙 정렬

      // 색상 그라데이션 (빨강 -> 주황 -> 노랑 -> 초록)
      color c;
      float gradPos = map(i, 0, numBars - 1, 0, 1);
      if (gradPos < 0.33) { // Red to Orange
          c = lerpColor(color(255, 0, 0), color(255, 165, 0), map(gradPos, 0, 0.33, 0, 1));
      } else if (gradPos < 0.66) { // Orange to Yellow
          c = lerpColor(color(255, 165, 0), color(255, 255, 0), map(gradPos, 0.33, 0.66, 0, 1));
      } else { // Yellow to Green
          c = lerpColor(color(255, 255, 0), color(0, 255, 0), map(gradPos, 0.66, 1, 0, 1));
      }
      pg.fill(c);
      pg.rect(barX, cy + maxBarHeight / 2 - barHeight, barWidth, barHeight);
    }
  } else if (name.equals("Date")) { // 날짜/시간 그리기 (이미지 참고)
    pg.fill(255, 0, 0); // 붉은색 텍스트
    pg.textSize(10); // 텍스트 크기

    // 현재 날짜와 시간 가져오기
    LocalDateTime now = LocalDateTime.now();
    // 이미지에 맞춰 포맷 변경 (25/ Jun/ 29)
    DateTimeFormatter dateFormatter = DateTimeFormatter.ofPattern("yy/ MMM/ dd");
    // 이미지에 맞춰 포맷 변경 (PM 05:30)
    DateTimeFormatter timeFormatter = DateTimeFormatter.ofPattern("a hh:mm"); // a는 오전/오후

    String dateStr = now.format(dateFormatter);
    String timeStr = now.format(timeFormatter);

    pg.text(dateStr, cx, cy - 10);
    pg.text(timeStr, cx, cy + 10);
  } else { // 텍스트 입력 처리 (기본 텍스트 색상 사용)
    // 텍스트 입력 처리: 줄바꿈 고려 및 줄 간격 최소화
    String[] lines = name.split("\n"); // 줄바꿈 문자로 텍스트 분리
    pg.textSize(10); // 사용자 요청대로 폰트 사이즈는 유지
    float lineHeight = pg.textAscent() + pg.textDescent(); // 폰트의 실제 높이
    float totalTextHeight = lineHeight * lines.length;
    float startY = cy - totalTextHeight / 2; // 전체 텍스트 블록의 시작 Y 좌표 (중앙 정렬)

    for (int i = 0; i < lines.length; i++) {
      pg.text(lines[i], cx, startY + i * lineHeight); // 각 줄을 그림
    }
  }

  pg.endDraw();

  // PGraphics의 픽셀을 grandMatrix로 복사
  pg.loadPixels();
  for (int y = 0; y < rows; y++) {
    for (int x = 0; x < cols * 2; x++) {
      grandMatrix[x][y] = pg.pixels[y * pg.width + x];
    }
  }
  dividePicture(); // grandMatrix를 두 개의 매트릭스로 분할
  updateBuffer(); // 버퍼 업데이트
}
