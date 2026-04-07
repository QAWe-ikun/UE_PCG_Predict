#include "Editor/FPCGPinHoverIntegration.h"
#include "Core/PCGPredictorEngine.h"
#include "Editor/PCGGraphActions.h"
#include "EdGraph/EdGraph.h"
#include "Framework/Application/SlateApplication.h"
#include "Framework/Docking/TabManager.h"
#include "HAL/PlatformTime.h"
#include "SGraphNode.h"
#include "SGraphPanel.h"
#include "UI/SPCGPredictionPopup.h"
#include "Widgets/SWindow.h"

void FPCGPinHoverIntegration::Initialize() {
  UE_LOG(LogTemp, Log, TEXT("========================================"));
  UE_LOG(LogTemp, Log, TEXT("PCG Pin Hover Integration Initialized"));
  UE_LOG(LogTemp, Log, TEXT("Using Slate polling with mouse position"));
  UE_LOG(LogTemp, Log, TEXT("========================================"));

  ConsecutiveDetectionCount = 0;
  LastHoverTime = 0.0;
  CurrentTargetPin = nullptr;
  CurrentNodeName = TEXT("");
  CurrentPinDirection = EGPD_Input;
  CurrentGraphPanel = nullptr;
  CurrentMousePosition = FVector2D::ZeroVector;

  UE_LOG(LogTemp, Log, TEXT("✓ Initialization complete"));
}

void FPCGPinHoverIntegration::Shutdown() {
  if (PredictionPopupWindow.IsValid()) {
    FSlateApplication::Get().DestroyWindowImmediately(
        PredictionPopupWindow.ToSharedRef());
    PredictionPopupWindow.Reset();
  }
}

void FPCGPinHoverIntegration::SetPredictorEngine(
    TSharedPtr<FPCGPredictorEngine> Engine) {
  PredictorEngine = Engine;
}

void FPCGPinHoverIntegration::SetOnCandidateClicked(
    FOnCandidateClicked Callback) {
  OnCandidateClickedCallback = MoveTemp(Callback);
}

// 从 SGraphNode 获取节点名称
FString GetNodeNameFromSGraphNode(TSharedPtr<SWidget> Widget) {
  if (!Widget.IsValid()) {
    return TEXT("Node");
  }

  TSharedPtr<SGraphNode> GraphNode = StaticCastSharedPtr<SGraphNode>(Widget);
  if (GraphNode.IsValid()) {
    FString NodeTitle = GraphNode->GetEditableNodeTitle();
    if (!NodeTitle.IsEmpty()) {
      UE_LOG(LogTemp, Log, TEXT("  Found node title: %s"), *NodeTitle);
      return NodeTitle;
    }
  }

  return TEXT("Node");
}

// 从 SGraphPin 获取 UEdGraphPin*
UEdGraphPin *GetUEdGraphPinFromWidget(TSharedPtr<SWidget> Widget) {
  if (!Widget.IsValid()) {
    return nullptr;
  }

  // 尝试转换为 SGraphPin
  TSharedPtr<SGraphPin> GraphPinWidget = StaticCastSharedPtr<SGraphPin>(Widget);
  if (GraphPinWidget.IsValid()) {
    return GraphPinWidget->GetPinObj();
  }

  return nullptr;
}

void FPCGPinHoverIntegration::DetectPinUnderCursor() {
  // 早期退出检查：如果功能被禁用，直接返回
  if (!bIsEnabled) {
    return;
  }

  // 检查 Slate 应用是否初始化
  if (!FSlateApplication::Get().IsInitialized()) {
    return;
  }

  // 获取鼠标位置
  FSlateApplication &Application = FSlateApplication::Get();
  FVector2D MousePos = Application.GetCursorPos();

  // 检查鼠标是否在预测窗口范围内
  if (PredictionPopupWindow.IsValid()) {
    FVector2D WindowPosition = PredictionPopupWindow->GetPositionInScreen();
    FVector2D WindowSize = PredictionPopupWindow->GetSizeInScreen();

    bool bMouseInWindowX = (MousePos.X >= WindowPosition.X &&
                            MousePos.X <= WindowPosition.X + WindowSize.X);
    bool bMouseInWindowY = (MousePos.Y >= WindowPosition.Y &&
                            MousePos.Y <= WindowPosition.Y + WindowSize.Y);

    if (bMouseInWindowX && bMouseInWindowY) {
      return;
    }
  }

  // 获取根窗口
  TSharedPtr<SWindow> RootWindow = FGlobalTabmanager::Get()->GetRootWindow();
  if (!RootWindow.IsValid()) {
    return;
  }

  TArray<TSharedRef<SWindow>> Windows;
  Windows.Add(RootWindow.ToSharedRef());

  FWidgetPath WidgetPath =
      FSlateApplication::Get().LocateWindowUnderMouse(MousePos, Windows);
  if (WidgetPath.Widgets.Num() == 0) {
    HidePrediction();
    ConsecutiveDetectionCount = 0;
    LastDetectedWidgetType = TEXT("");
    CurrentTargetPin = nullptr;
    CurrentGraphPanel = nullptr;
    return;
  }

  // 获取最上层的 Widget
  TSharedPtr<SWidget> WidgetUnderMouse = WidgetPath.Widgets.Last().Widget;
  if (!WidgetUnderMouse.IsValid()) {
    HidePrediction();
    ConsecutiveDetectionCount = 0;
    LastDetectedWidgetType = TEXT("");
    CurrentTargetPin = nullptr;
    CurrentGraphPanel = nullptr;
    return;
  }

  // 从鼠标下的 Widget 开始向上查找 Pin 类型或 GraphPanel
  TSharedPtr<SWidget> CurrentWidget = WidgetUnderMouse;
  int32 Depth = 0;
  TSharedPtr<SGraphPanel> FoundGraphPanel = nullptr;

  while (CurrentWidget.IsValid() && Depth < 20) {
    const FName CurrentWidgetType = CurrentWidget->GetType();
    FString CurrentTypeName = CurrentWidgetType.ToString();
    FString LowerTypeName = CurrentTypeName.ToLower();

    // 检查是否是 GraphPanel（空白区域）
    if (LowerTypeName.Contains(TEXT("graphpanel"))) {
      FoundGraphPanel = StaticCastSharedPtr<SGraphPanel>(CurrentWidget);

      if (FoundGraphPanel.IsValid()) {
        // 检查 Graph 中是否已经有节点（除了 Input/Output）
        UEdGraph* EdGraph = FoundGraphPanel->GetGraphObj();
        if (EdGraph) {
          UE_LOG(LogTemp, Log, TEXT("  Checking graph nodes, total: %d"), EdGraph->Nodes.Num());
          int32 UserNodeCount = 0;
          for (UEdGraphNode* Node : EdGraph->Nodes) {
            if (Node) {
              FString NodeTitle = Node->GetNodeTitle(ENodeTitleType::ListView).ToString();
              UE_LOG(LogTemp, Log, TEXT("  Node title: '%s'"), *NodeTitle);

              // 排除 InputNode、OutputNode 以及中文的"输入"、"输出"
              if (NodeTitle != TEXT("InputNode") &&
                  NodeTitle != TEXT("OutputNode") &&
                  NodeTitle != TEXT("输入") &&
                  NodeTitle != TEXT("输出") &&
                  NodeTitle != TEXT("Input") &&
                  NodeTitle != TEXT("Output")) {
                UserNodeCount++;
                UE_LOG(LogTemp, Log, TEXT("  -> Counted as user node"));
              } else {
                UE_LOG(LogTemp, Log, TEXT("  -> Excluded (Input/Output node)"));
              }
            }
          }

          // 如果已经有用户创建的节点，不显示空白区域预测
          if (UserNodeCount > 0) {
            UE_LOG(LogTemp, Log, TEXT("  Graph has %d user nodes, skipping empty area prediction"), UserNodeCount);
            HidePrediction();
            ConsecutiveDetectionCount = 0;
            LastDetectedWidgetType = TEXT("");
            return;
          }

          UE_LOG(LogTemp, Log, TEXT("  Graph is empty (only Input/Output nodes), showing starter prediction"));
        }

        ConsecutiveDetectionCount++;

        if (ConsecutiveDetectionCount < 3) {
          return;
        }

        // 如果窗口已存在，持续更新位置
        if (PredictionPopupWindow.IsValid()) {
          FVector2D NewPos(MousePos.X + 20, MousePos.Y + 20);
          FVector2D CurrentWindowPos = PredictionPopupWindow->GetPositionInScreen();
          UE_LOG(LogTemp, Log, TEXT("[WindowTracking-Empty] Mouse: (%.1f, %.1f), Window: (%.1f, %.1f) -> Moving to (%.1f, %.1f)"),
                 MousePos.X, MousePos.Y, CurrentWindowPos.X, CurrentWindowPos.Y, NewPos.X, NewPos.Y);
          PredictionPopupWindow->MoveWindowTo(NewPos);
        }

        // 防抖：检查是否与上次相同
        if (LastDetectedWidgetType == TEXT("GraphPanel")) {
          // 窗口已存在，只更新了位置，不需要重新创建
          return;
        }

        LastDetectedWidgetType = TEXT("GraphPanel");

        // 显示起始节点预测（没有输入pin的节点）
        UE_LOG(LogTemp, Log, TEXT("  Detected GraphPanel empty area (no user nodes)"));
        ShowStarterNodePrediction(FoundGraphPanel, MousePos);
        return;
      }
    }

    // 检查是否是 Pin 类型
    bool bIsPin = LowerTypeName.Contains(TEXT("pin")) &&
                  (LowerTypeName.Contains(TEXT("graph")) ||
                   LowerTypeName.Contains(TEXT("node"))) &&
                  !LowerTypeName.Contains(TEXT("panel")) &&
                  !LowerTypeName.Contains(TEXT("window")) &&
                  !LowerTypeName.Contains(TEXT("border")) &&
                  !LowerTypeName.Contains(TEXT("box")) &&
                  !LowerTypeName.Contains(TEXT("area"));

    if (bIsPin) {
      // 尝试获取节点名称
      FString NodeName = TEXT("Node");
      TSharedPtr<SWidget> ParentWidget = CurrentWidget->GetParentWidget();

      // 向上查找最多 15 层，寻找 GraphNode 和 GraphPanel
      for (int32 i = 0; i < 15 && ParentWidget.IsValid(); i++) {
        FString ParentTypeName = ParentWidget->GetType().ToString();

        // 查找 GraphNode
        if (ParentTypeName.Contains(TEXT("GraphNode")) ||
            ParentTypeName.Contains(TEXT("PCGEditorGraphNode"))) {
          // 使用 GetEditableNodeTitle() 获取节点名称
          NodeName = GetNodeNameFromSGraphNode(ParentWidget);

          // 如果还是 "Node"，使用后备名称
          if (NodeName == TEXT("Node")) {
            NodeName = TEXT("PCG Node");
          }

          // 从 GraphNode 获取 GraphPanel
          TSharedPtr<SGraphNode> GraphNodeWidget =
              StaticCastSharedPtr<SGraphNode>(ParentWidget);
          if (GraphNodeWidget.IsValid()) {
            FoundGraphPanel = GraphNodeWidget->GetOwnerPanel();
          }
        }

        ParentWidget = ParentWidget->GetParentWidget();
      }

      ConsecutiveDetectionCount++;

      if (ConsecutiveDetectionCount < 3) {
        return;
      }

      // 如果窗口已存在，持续更新位置
      if (PredictionPopupWindow.IsValid()) {
        FVector2D NewPos(MousePos.X + 20, MousePos.Y + 20);
        FVector2D CurrentWindowPos = PredictionPopupWindow->GetPositionInScreen();
        UE_LOG(LogTemp, Log, TEXT("[WindowTracking] Mouse: (%.1f, %.1f), Window: (%.1f, %.1f) -> Moving to (%.1f, %.1f)"),
               MousePos.X, MousePos.Y, CurrentWindowPos.X, CurrentWindowPos.Y, NewPos.X, NewPos.Y);
        PredictionPopupWindow->MoveWindowTo(NewPos);
      } else {
        UE_LOG(LogTemp, Warning, TEXT("[WindowTracking] Window is not valid, cannot update position"));
      }

      // 防抖
      double CurrentTime = FPlatformTime::Seconds();
      if (CurrentTime - LastHoverTime < 0.3) {
        UE_LOG(LogTemp, Verbose, TEXT("[WindowTracking] In debounce period, skipping recreation"));
        return;
      }

      LastHoverTime = CurrentTime;

      if (CurrentTypeName == LastDetectedWidgetType &&
          PredictionPopupWindow.IsValid()) {
        // 窗口已存在且是同一个pin，不需要重新创建
        UE_LOG(LogTemp, Verbose, TEXT("[WindowTracking] Same pin detected, window exists, no recreation needed"));
        return;
      }

      LastDetectedWidgetType = CurrentTypeName;

      FString PinName = TEXT("Pin");
      FString Direction = TEXT("Output"); // 默认值

      FString AccessibleText = CurrentWidget->GetAccessibleText().ToString();
      if (!AccessibleText.IsEmpty()) {
        PinName = AccessibleText;
      }

      // 获取 UEdGraphPin*
      UEdGraphPin *PinObj = GetUEdGraphPinFromWidget(CurrentWidget);

      // 从 Pin 对象获取真实方向
      if (PinObj) {
        Direction = (PinObj->Direction == EGPD_Input) ? TEXT("Input") : TEXT("Output");
        UE_LOG(LogTemp, Log, TEXT("  Pin direction from object: %s"), *Direction);
      }

      // 保存当前目标 Pin 信息
      CurrentNodeName = NodeName;
      CurrentPinDirection =
          (Direction == TEXT("Input")) ? EGPD_Input : EGPD_Output;
      CurrentTargetPin = PinObj;
      CurrentGraphPanel = FoundGraphPanel;
      CurrentMousePosition = MousePos;

      ShowPrediction(PinName, Direction, NodeName, FoundGraphPanel, PinObj);
      return;
    }

    CurrentWidget = CurrentWidget->GetParentWidget();
    Depth++;
  }

  // 如果鼠标不在 Pin 上且不在预测窗口上，则隐藏预测
  HidePrediction();
  ConsecutiveDetectionCount = 0;
  LastDetectedWidgetType = TEXT("");
  CurrentTargetPin = nullptr;
  CurrentGraphPanel = nullptr;
}

void FPCGPinHoverIntegration::ShowPrediction(const FString &PinName,
                                             const FString &Direction,
                                             const FString &NodeName,
                                             TSharedPtr<SGraphPanel> GraphPanel,
                                             UEdGraphPin *Pin) {
  UE_LOG(LogTemp, Log, TEXT("==========================================="));
  UE_LOG(LogTemp, Log, TEXT(">>> Show Prediction: %s.%s (%s)"), *NodeName,
         *PinName, *Direction);

  // 先获取预测数据
  TArray<FPCGCandidate> Candidates;
  TArray<FString> ConnectedNodes;

  if (PredictorEngine.IsValid()) {
    EPCGPredictPinDirection PredictDirection = GetPredictDirection(Direction);
    Candidates = PredictorEngine->Predict(PredictDirection, Pin);

    // 获取已连接的节点信息
    if (Pin) {
      for (UEdGraphPin* LinkedPin : Pin->LinkedTo) {
        if (LinkedPin && LinkedPin->GetOwningNode()) {
          FString NodeTitle = LinkedPin->GetOwningNode()->GetNodeTitle(ENodeTitleType::ListView).ToString();
          if (!NodeTitle.IsEmpty()) {
            ConnectedNodes.Add(NodeTitle);
          }
        }
      }
    }

    UE_LOG(LogTemp, Log, TEXT("  Got %d candidates from predictor, %d connected nodes"),
           Candidates.Num(), ConnectedNodes.Num());

    // 打印候选列表
    for (int32 i = 0; i < Candidates.Num(); i++) {
      UE_LOG(LogTemp, Log, TEXT("    [%d] %s (%.2f)"), i,
             *Candidates[i].NodeTypeName, Candidates[i].Score);
    }
  } else {
    UE_LOG(LogTemp, Warning, TEXT("  PredictorEngine is not valid!"));
  }

  // 创建或更新窗口
  // 强制销毁旧窗口，避免状态残留
  if (PredictionPopupWindow.IsValid()) {
    UE_LOG(LogTemp, Log, TEXT("  Destroying existing popup window..."));
    FSlateApplication::Get().DestroyWindowImmediately(
        PredictionPopupWindow.ToSharedRef());
    PredictionPopupWindow.Reset();
  }

  UE_LOG(LogTemp, Log, TEXT("  Creating new popup window..."));
  CreatePredictionPopup();

  // 更新窗口内容
  if (PredictionPopupWindow.IsValid()) {
    TSharedPtr<SWidget> Content = PredictionPopupWindow->GetContent();
    if (Content.IsValid()) {
      TSharedPtr<SPCGPredictionPopup> Popup =
          StaticCastSharedPtr<SPCGPredictionPopup>(Content);

      if (Popup.IsValid()) {
        UE_LOG(LogTemp, Log, TEXT("  Updating popup content..."));
        Popup->SetPredictorEngine(PredictorEngine.Get());
        Popup->UpdatePredictions(Candidates, GetPredictDirection(Direction), ConnectedNodes);
        Popup->SetSelectedNodeName(NodeName);

        // 设置点击回调 - 现在包含完整的 GraphPanel 和 Pin 信息
        Popup->SetOnCandidateClicked([GraphPanel, Pin,
                                      PinDirection =
                                          GetPredictDirection(Direction)](
                                         const FPCGCandidate &Candidate,
                                         int32 Index) {
          UE_LOG(LogTemp, Log, TEXT("[Callback] Candidate clicked: %s (%d)"),
                 *Candidate.NodeTypeName, Index);

          // 创建节点并连接
          if (GraphPanel.IsValid() && Pin) {
            UE_LOG(LogTemp, Log, TEXT("[Callback] Creating node: %s"),
                   *Candidate.NodeTypeName);

            // 计算新节点位置：基于源节点位置
            FVector2D SpawnLocation = FVector2D::ZeroVector;

            if (Pin->GetOwningNode()) {
              UEdGraphNode* OwningNode = Pin->GetOwningNode();
              FVector2D NodePos(OwningNode->NodePosX, OwningNode->NodePosY);

              // 根据pin方向决定偏移
              // 输出pin -> 新节点在右侧
              // 输入pin -> 新节点在下方
              if (PinDirection == EPCGPredictPinDirection::Output) {
                SpawnLocation = FVector2D(NodePos.X + 400.0f, NodePos.Y);
              } else {
                SpawnLocation = FVector2D(NodePos.X, NodePos.Y + 200.0f);
              }

              UE_LOG(LogTemp, Log, TEXT("[Callback] Source node at (%.1f, %.1f), new node at (%.1f, %.1f)"),
                     NodePos.X, NodePos.Y, SpawnLocation.X, SpawnLocation.Y);
            } else {
              // 后备方案：使用鼠标位置
              SpawnLocation = FVector2D(100.0f, 100.0f);
              UE_LOG(LogTemp, Warning, TEXT("[Callback] No owning node, using fallback position"));
            }

            // 转换 EPCGPredictPinDirection 为 EEdGraphPinDirection
            EEdGraphPinDirection EdDirection =
                (PinDirection == EPCGPredictPinDirection::Input) ? EGPD_Input
                                                                 : EGPD_Output;

            bool Success = FPCGGraphActions::CreateNodeAndConnect(
                GraphPanel, Candidate.NodeTypeName, Pin, EdDirection,
                SpawnLocation);

            if (Success) {
              UE_LOG(
                  LogTemp, Log,
                  TEXT("[Callback] Node created and connected successfully!"));
            } else {
              UE_LOG(LogTemp, Error, TEXT("[Callback] Failed to create node"));
            }
          } else {
            UE_LOG(LogTemp, Warning,
                   TEXT("[Callback] GraphPanel or Pin is null"));
          }
        });

        UE_LOG(LogTemp, Log, TEXT("  Set node name: %s"), *NodeName);
      } else {
        UE_LOG(LogTemp, Warning,
               TEXT("  Failed to cast to SPCGPredictionPopup"));
      }
    } else {
      UE_LOG(LogTemp, Warning, TEXT("  Popup content is not valid"));
    }
  } else {
    UE_LOG(LogTemp, Warning,
           TEXT("  PredictionPopupWindow is not valid after creation"));
  }

  // 显示窗口
  if (PredictionPopupWindow.IsValid()) {
    FVector2D MousePos = FSlateApplication::Get().GetCursorPos();
    PredictionPopupWindow->MoveWindowTo(
        FIntPoint(MousePos.X - 175, MousePos.Y));
    PredictionPopupWindow->ShowWindow();
    UE_LOG(LogTemp, Log, TEXT(">>> Prediction window shown at %.1f, %.1f"),
           MousePos.X, MousePos.Y);
    UE_LOG(LogTemp, Log, TEXT("==========================================="));
  } else {
    UE_LOG(LogTemp, Error, TEXT("==========================================="));
    UE_LOG(LogTemp, Error, TEXT("ERROR: Failed to show prediction window!"));
    UE_LOG(LogTemp, Error, TEXT("==========================================="));
  }
}

void FPCGPinHoverIntegration::HidePrediction() {
  if (PredictionPopupWindow.IsValid()) {
    FSlateApplication::Get().DestroyWindowImmediately(
        PredictionPopupWindow.ToSharedRef());
    PredictionPopupWindow.Reset();
    UE_LOG(LogTemp, Log, TEXT("<<< Prediction window destroyed"));
  }
}

void FPCGPinHoverIntegration::ShowStarterNodePrediction(TSharedPtr<SGraphPanel> GraphPanel, FVector2D MousePos) {
  UE_LOG(LogTemp, Log, TEXT("==========================================="));
  UE_LOG(LogTemp, Log, TEXT(">>> Show Starter Node Prediction (Empty Area)"));

  // 获取起始节点预测（没有输入pin的节点）
  TArray<FPCGCandidate> Candidates;
  if (PredictorEngine.IsValid()) {
    Candidates = PredictorEngine->PredictStarterNodes();
    UE_LOG(LogTemp, Log, TEXT("  Got %d starter node candidates"), Candidates.Num());

    for (int32 i = 0; i < Candidates.Num(); i++) {
      UE_LOG(LogTemp, Log, TEXT("    [%d] %s (%.2f)"), i,
             *Candidates[i].NodeTypeName, Candidates[i].Score);
    }
  } else {
    UE_LOG(LogTemp, Warning, TEXT("  PredictorEngine is not valid!"));
  }

  // 强制销毁旧窗口
  if (PredictionPopupWindow.IsValid()) {
    UE_LOG(LogTemp, Log, TEXT("  Destroying existing popup window..."));
    FSlateApplication::Get().DestroyWindowImmediately(
        PredictionPopupWindow.ToSharedRef());
    PredictionPopupWindow.Reset();
  }

  UE_LOG(LogTemp, Log, TEXT("  Creating new popup window..."));
  CreatePredictionPopup();

  // 更新窗口内容
  if (PredictionPopupWindow.IsValid()) {
    TSharedPtr<SWidget> Content = PredictionPopupWindow->GetContent();
    if (Content.IsValid()) {
      TSharedPtr<SPCGPredictionPopup> Popup =
          StaticCastSharedPtr<SPCGPredictionPopup>(Content);

      if (Popup.IsValid()) {
        UE_LOG(LogTemp, Log, TEXT("  Updating popup content..."));
        Popup->SetPredictorEngine(PredictorEngine.Get());

        // 空数组表示没有已连接节点
        TArray<FString> EmptyConnections;
        Popup->UpdatePredictions(Candidates, EPCGPredictPinDirection::Output, EmptyConnections);
        Popup->SetSelectedNodeName(TEXT("Empty Area"));

        // 设置点击回调 - 在空白区域创建节点
        Popup->SetOnCandidateClicked([GraphPanel](
                                         const FPCGCandidate &Candidate,
                                         int32 Index) {
          UE_LOG(LogTemp, Log, TEXT("[Callback] Starter node clicked: %s (%d)"),
                 *Candidate.NodeTypeName, Index);

          if (GraphPanel.IsValid()) {
            UE_LOG(LogTemp, Log, TEXT("[Callback] Creating starter node: %s"),
                   *Candidate.NodeTypeName);

            // 查找输出节点的位置
            FVector2D SpawnLocation = FVector2D(400.0f, 0.0f); // 默认位置
            UEdGraph* EdGraph = GraphPanel->GetGraphObj();

            if (EdGraph) {
              for (UEdGraphNode* Node : EdGraph->Nodes) {
                if (Node) {
                  FString NodeTitle = Node->GetNodeTitle(ENodeTitleType::ListView).ToString();
                  // 找到输出节点
                  if (NodeTitle == TEXT("输出") || NodeTitle == TEXT("Output") || NodeTitle == TEXT("OutputNode")) {
                    FVector2D OutputNodePos(Node->NodePosX, Node->NodePosY);
                    // 在输出节点右边创建
                    SpawnLocation = FVector2D(OutputNodePos.X + 400.0f, OutputNodePos.Y);
                    UE_LOG(LogTemp, Log, TEXT("[Callback] Found Output node at (%.1f, %.1f), creating at (%.1f, %.1f)"),
                           OutputNodePos.X, OutputNodePos.Y, SpawnLocation.X, SpawnLocation.Y);
                    break;
                  }
                }
              }
            }

            // 创建节点但不连接
            bool Success = FPCGGraphActions::CreateNodeAndConnect(
                GraphPanel, Candidate.NodeTypeName, nullptr, EGPD_Output,
                SpawnLocation);

            if (Success) {
              UE_LOG(LogTemp, Log, TEXT("[Callback] Starter node created successfully!"));
            } else {
              UE_LOG(LogTemp, Warning, TEXT("[Callback] Failed to create starter node"));
            }
          } else {
            UE_LOG(LogTemp, Warning, TEXT("[Callback] GraphPanel is null"));
          }
        });

        UE_LOG(LogTemp, Log, TEXT("  Set node name: Empty Area"));
      } else {
        UE_LOG(LogTemp, Warning,
               TEXT("  Failed to cast to SPCGPredictionPopup"));
      }
    } else {
      UE_LOG(LogTemp, Warning, TEXT("  Popup content is not valid"));
    }
  } else {
    UE_LOG(LogTemp, Error, TEXT("Failed to create prediction window!"));
    return;
  }

  // 显示窗口
  if (PredictionPopupWindow.IsValid()) {
    FVector2D WindowPos = MousePos + FVector2D(20, 20);
    PredictionPopupWindow->MoveWindowTo(WindowPos);
    PredictionPopupWindow->ShowWindow();
    UE_LOG(LogTemp, Log, TEXT(">>> Starter node prediction window shown at %.1f, %.1f"),
           WindowPos.X, WindowPos.Y);
    UE_LOG(LogTemp, Log, TEXT("==========================================="));
  } else {
    UE_LOG(LogTemp, Error, TEXT("==========================================="));
    UE_LOG(LogTemp, Error, TEXT("ERROR: Failed to show starter node prediction window!"));
    UE_LOG(LogTemp, Error, TEXT("==========================================="));
  }
}

void FPCGPinHoverIntegration::TriggerPredictionFromToolbar() {
  UE_LOG(LogTemp, Log, TEXT("Trigger prediction from toolbar"));

  if (!PredictionPopupWindow.IsValid()) {
    CreatePredictionPopup();
  }

  if (PredictorEngine.IsValid()) {
    TArray<FPCGCandidate> Candidates =
        PredictorEngine->Predict(EPCGPredictPinDirection::Output);

    if (PredictionPopupWindow.IsValid()) {
      TSharedPtr<SWidget> Content = PredictionPopupWindow->GetContent();
      if (Content.IsValid()) {
        TSharedPtr<SPCGPredictionPopup> Popup =
            StaticCastSharedPtr<SPCGPredictionPopup>(Content);

        if (Popup.IsValid()) {
          Popup->SetPredictorEngine(PredictorEngine.Get());
          Popup->UpdatePredictions(Candidates, EPCGPredictPinDirection::Output);
        }
      }
    }

    if (PredictionPopupWindow.IsValid()) {
      FVector2D MousePos = FSlateApplication::Get().GetCursorPos();
      PredictionPopupWindow->MoveWindowTo(
          FIntPoint(MousePos.X - 175, MousePos.Y));
      PredictionPopupWindow->ShowWindow();
    }
  }
}

void FPCGPinHoverIntegration::CreatePredictionPopup() {
  PredictionPopupWindow =
      SNew(SWindow)
          .ClientSize(FVector2D(350, 400))
          .AutoCenter(EAutoCenter::None)
          .SupportsMaximize(false)
          .SupportsMinimize(false)
          .SizingRule(ESizingRule::Autosized)
          .IsTopmostWindow(true)[SNew(SPCGPredictionPopup).MaxCandidates(5)];

  TSharedPtr<SWidget> Content = PredictionPopupWindow->GetContent();
  if (Content.IsValid()) {
    TSharedPtr<SPCGPredictionPopup> Popup =
        StaticCastSharedPtr<SPCGPredictionPopup>(Content);
    if (Popup.IsValid()) {
      Popup->SetPopupWindow(PredictionPopupWindow);
      UE_LOG(LogTemp, Log, TEXT("Created prediction popup window"));
    }
  }

  FSlateApplication::Get().AddWindow(PredictionPopupWindow.ToSharedRef());
}

void FPCGPinHoverIntegration::UpdatePredictionContent(
    const FString &PinName, const FString &Direction) {
  if (!PredictorEngine.IsValid()) {
    UE_LOG(LogTemp, Warning,
           TEXT("UpdatePredictionContent: PredictorEngine not valid"));
    return;
  }

  EPCGPredictPinDirection PredictDirection = GetPredictDirection(Direction);
  TArray<FPCGCandidate> Candidates = PredictorEngine->Predict(PredictDirection);

  UE_LOG(LogTemp, Log, TEXT("UpdatePredictionContent: Got %d candidates"),
         Candidates.Num());

  if (PredictionPopupWindow.IsValid()) {
    TSharedPtr<SWidget> Content = PredictionPopupWindow->GetContent();
    if (Content.IsValid()) {
      TSharedPtr<SPCGPredictionPopup> Popup =
          StaticCastSharedPtr<SPCGPredictionPopup>(Content);

      if (Popup.IsValid()) {
        Popup->SetPredictorEngine(PredictorEngine.Get());
        Popup->UpdatePredictions(Candidates, PredictDirection);
        UE_LOG(
            LogTemp, Log,
            TEXT("UpdatePredictionContent: Updated popup with %d candidates"),
            Candidates.Num());
      }
    }
  }
}

EPCGPredictPinDirection
FPCGPinHoverIntegration::GetPredictDirection(const FString &Direction) {
  return (Direction == TEXT("Input") || Direction == TEXT("EGPD_Input"))
             ? EPCGPredictPinDirection::Input
             : EPCGPredictPinDirection::Output;
}

void FPCGPinHoverIntegration::SetEnabled(bool bEnabled) {
  bIsEnabled = bEnabled;

  // 如果禁用，立即隐藏预测窗口
  if (!bEnabled) {
    HidePrediction();
  }

  UE_LOG(LogTemp, Log, TEXT("[PCGPinHoverIntegration] Prediction %s"),
         bEnabled ? TEXT("enabled") : TEXT("disabled"));
}
