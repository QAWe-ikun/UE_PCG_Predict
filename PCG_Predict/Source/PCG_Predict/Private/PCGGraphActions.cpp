#include "PCGGraphActions.h"
#include "EdGraph/EdGraph.h"
#include "EdGraph/EdGraphNode.h"
#include "EdGraph/EdGraphPin.h"
#include "EdGraph/EdGraphSchema.h"
#include "GraphEditor.h"
#include "SGraphPanel.h"

// PCG 核心头文件
#include "PCGGraph.h"
#include "PCGNode.h"
#include "PCGSettings.h"

#define LOCTEXT_NAMESPACE "PCGGraphActions"

bool FPCGGraphActions::CreateNodeAndConnect(TSharedPtr<SGraphPanel> GraphPanel,
                                            const FString &NodeTypeName,
                                            UEdGraphPin *TargetPin,
                                            EEdGraphPinDirection Direction,
                                            FVector2D SpawnLocation) {
  if (!GraphPanel.IsValid() || !TargetPin) {
    UE_LOG(LogTemp, Error,
           TEXT("[PCGGraphActions] Invalid GraphPanel or TargetPin"));
    return false;
  }

  // 从 TargetPin 获取 Graph
  UEdGraphNode *OuterNode = Cast<UEdGraphNode>(TargetPin->GetOuter());
  if (!OuterNode) {
    UE_LOG(LogTemp, Error,
           TEXT("[PCGGraphActions] Cannot get outer node from pin"));
    return false;
  }

  UEdGraph *EdGraph = OuterNode->GetGraph();
  if (!EdGraph) {
    UE_LOG(LogTemp, Error,
           TEXT("[PCGGraphActions] Cannot get graph from node"));
    return false;
  }

  // 获取 PCG Graph - 从 EdGraph 的 Outer 获取
  UPCGGraph *PCGGraph = nullptr;

  UObject *Outer = EdGraph->GetOuter();
  if (Outer) {
    PCGGraph = Cast<UPCGGraph>(Outer);
  }

  if (!PCGGraph) {
    UE_LOG(LogTemp, Error, TEXT("[PCGGraphActions] Not a PCG Graph"));
    return false;
  }

  UE_LOG(LogTemp, Log,
         TEXT("[PCGGraphActions] Creating node: %s at %.1f, %.1f"),
         *NodeTypeName, SpawnLocation.X, SpawnLocation.Y);

  // 1. 创建 Settings
  UClass *SettingsClass = FindPCGSettingsClass(NodeTypeName);
  if (!SettingsClass) {
    UE_LOG(LogTemp, Error,
           TEXT("[PCGGraphActions] Settings class not found for: %s"),
           *NodeTypeName);
    return false;
  }

  // 2. 使用事务创建节点（支持 Undo/Redo）
  const FScopedTransaction Transaction(
      LOCTEXT("PCGGraphActions_CreateNode", "Create PCG Node"));

  // 标记 EdGraph 已修改
  EdGraph->Modify();

  // 3. 创建数据层节点
  UPCGSettings *Settings = NewObject<UPCGSettings>(PCGGraph, SettingsClass);
  if (!Settings) {
    UE_LOG(LogTemp, Error, TEXT("[PCGGraphActions] Failed to create Settings"));
    return false;
  }

  UPCGNode *NewPCGNode = PCGGraph->AddNode(Settings);
  if (!NewPCGNode) {
    UE_LOG(LogTemp, Error,
           TEXT("[PCGGraphActions] Failed to add node to graph"));
    return false;
  }

// 4. 设置节点位置
#if WITH_EDITOR
  NewPCGNode->SetNodePosition((int32)SpawnLocation.X, (int32)SpawnLocation.Y);
#endif

  // 5. 创建 Editor Graph Node（关键！）
  // 直接使用 EdGraph 创建 Editor Graph Node
  UEdGraphNode *NewEditorNode = nullptr;
  {
    FGraphNodeCreator<UEdGraphNode> NodeCreator(*EdGraph);
    NewEditorNode = NodeCreator.CreateNode(false);
    if (NewEditorNode) {
      NewEditorNode->NodePosX = SpawnLocation.X;
      NewEditorNode->NodePosY = SpawnLocation.Y;
      NodeCreator.Finalize();

      // 添加到 Graph 的节点数组
      EdGraph->AddNode(NewEditorNode, false, false);

      UE_LOG(LogTemp, Log,
             TEXT("[PCGGraphActions] Created Editor Graph Node: %s at (%.1f, "
                  "%.1f)"),
             *NewEditorNode->GetClass()->GetName(), SpawnLocation.X,
             SpawnLocation.Y);
    }
  }

  UE_LOG(LogTemp, Log, TEXT("[PCGGraphActions] PCG Node created: %s"),
         *NodeTypeName);

  // 6. 选中并聚焦到新节点
  if (GraphPanel.IsValid() && NewEditorNode) {
    GraphPanel->SelectionManager.ClearSelectionSet();
    GraphPanel->SelectionManager.SetNodeSelection(NewEditorNode, true);
  }

  UE_LOG(LogTemp, Log, TEXT("[PCGGraphActions] Node created successfully"));

  return true;
}

UPCGNode *FPCGGraphActions::CreatePCGNode(UPCGGraph *Graph,
                                          const FString &NodeTypeName,
                                          FVector2D Location) {
  if (!Graph) {
    UE_LOG(LogTemp, Error, TEXT("[PCGGraphActions] Invalid Graph"));
    return nullptr;
  }

  UClass *SettingsClass = FindPCGSettingsClass(NodeTypeName);
  if (!SettingsClass) {
    UE_LOG(LogTemp, Error,
           TEXT("[PCGGraphActions] Settings class not found for: %s"),
           *NodeTypeName);
    return nullptr;
  }

  UPCGSettings *Settings = NewObject<UPCGSettings>(Graph, SettingsClass);
  if (!Settings) {
    UE_LOG(LogTemp, Error, TEXT("[PCGGraphActions] Failed to create Settings"));
    return nullptr;
  }

  UPCGNode *NewNode = Graph->AddNode(Settings);
  if (!NewNode) {
    UE_LOG(LogTemp, Error,
           TEXT("[PCGGraphActions] Failed to add node to graph"));
    return nullptr;
  }

#if WITH_EDITOR
  NewNode->SetNodePosition((int32)Location.X, (int32)Location.Y);
#endif

  return NewNode;
}

UClass *FPCGGraphActions::FindPCGSettingsClass(const FString &NodeTypeName) {
  static TMap<FString, UClass *> SettingsClassMap;

  if (SettingsClassMap.Num() == 0) {
    // Common nodes
    SettingsClassMap.Add(TEXT("Difference"),
                         FindClass(TEXT("/Script/PCG.PCGDifferenceSettings")));
    SettingsClassMap.Add(TEXT("Union"),
                         FindClass(TEXT("/Script/PCG.PCGUnionSettings")));
    SettingsClassMap.Add(
        TEXT("Intersection"),
        FindClass(TEXT("/Script/PCG.PCGOuterIntersectionSettings")));

    // Samplers
    SettingsClassMap.Add(
        TEXT("SurfaceSampler"),
        FindClass(TEXT("/Script/PCG.PCGSurfaceSamplerSettings")));
    SettingsClassMap.Add(
        TEXT("SplineSampler"),
        FindClass(TEXT("/Script/PCG.PCGSplineSamplerSettings")));
    SettingsClassMap.Add(TEXT("GridSampler"),
                         FindClass(TEXT("/Script/PCG.PCGGridSamplerSettings")));
    SettingsClassMap.Add(
        TEXT("VolumeSampler"),
        FindClass(TEXT("/Script/PCG.PCGVolumeSamplerSettings")));

    // Transform
    SettingsClassMap.Add(
        TEXT("TransformPoints"),
        FindClass(TEXT("/Script/PCG.PCGTransformPointsSettings")));
    SettingsClassMap.Add(
        TEXT("OrientPointsToSurface"),
        FindClass(TEXT("/Script/PCG.PCGOrientPointsToSurfaceSettings")));

    // Filter
    SettingsClassMap.Add(
        TEXT("DensityFilter"),
        FindClass(TEXT("/Script/PCG.PCGDensityFilterSettings")));
    SettingsClassMap.Add(TEXT("FilterByTag"),
                         FindClass(TEXT("/Script/PCG.PCGFilterByTagSettings")));
    SettingsClassMap.Add(
        TEXT("RandomSelection"),
        FindClass(TEXT("/Script/PCG.PCGRandomSelectionSettings")));

    // Spawner
    SettingsClassMap.Add(
        TEXT("StaticMeshSpawner"),
        FindClass(TEXT("/Script/PCG.PCGStaticMeshSpawnerSettings")));

    // Attribute
    SettingsClassMap.Add(
        TEXT("AttributeTransfer"),
        FindClass(TEXT("/Script/PCG.PCGAttributeTransferSettings")));
    SettingsClassMap.Add(
        TEXT("SetAttributes"),
        FindClass(TEXT("/Script/PCG.PCGSetAttributesSettings")));

    // Spatial
    SettingsClassMap.Add(TEXT("FindFloor"),
                         FindClass(TEXT("/Script/PCG.PCGFindFloorSettings")));
    SettingsClassMap.Add(
        TEXT("RemoveBottom"),
        FindClass(TEXT("/Script/PCG.PCGRemoveBottomSettings")));

    // Graph
    SettingsClassMap.Add(TEXT("Graph"),
                         FindClass(TEXT("/Script/PCG.PCGGraphSettings")));
    SettingsClassMap.Add(TEXT("MultiGraph"),
                         FindClass(TEXT("/Script/PCG.PCGMultiGraphSettings")));

    // Create Points
    SettingsClassMap.Add(
        TEXT("CreatePoints"),
        FindClass(TEXT("/Script/PCG.PCGCreatePointsSettings")));
    SettingsClassMap.Add(
        TEXT("CreatePointsGrid"),
        FindClass(TEXT("/Script/PCG.PCGCreatePointsGridSettings")));

    // Debug
    SettingsClassMap.Add(TEXT("Debug"),
                         FindClass(TEXT("/Script/PCG.PCGDebugSettings")));
    SettingsClassMap.Add(
        TEXT("PrintString"),
        FindClass(TEXT("/Script/PCG.PCGPrintElementSettings")));

    // Control Flow
    SettingsClassMap.Add(TEXT("Branch"),
                         FindClass(TEXT("/Script/PCG.PCGBranchSettings")));
    SettingsClassMap.Add(TEXT("Switch"),
                         FindClass(TEXT("/Script/PCG.PCGSwitchSettings")));

    // Metadata
    SettingsClassMap.Add(
        TEXT("AttributeCast"),
        FindClass(TEXT("/Script/PCG.PCGAttributeCastSettings")));
    SettingsClassMap.Add(
        TEXT("AttributeFilter"),
        FindClass(TEXT("/Script/PCG.PCGAttributeFilteringSettings")));
    SettingsClassMap.Add(
        TEXT("AttributeRemap"),
        FindClass(TEXT("/Script/PCG.PCGAttributeRemapSettings")));
    SettingsClassMap.Add(
        TEXT("AttributeNoise"),
        FindClass(TEXT("/Script/PCG.PCGAttributeNoiseSettings")));

    // More common nodes
    SettingsClassMap.Add(
        TEXT("AddComponent"),
        FindClass(TEXT("/Script/PCG.PCGAddComponentSettings")));
    SettingsClassMap.Add(TEXT("AddTags"),
                         FindClass(TEXT("/Script/PCG.PCGAddTagSettings")));
    SettingsClassMap.Add(
        TEXT("ApplyHierarchy"),
        FindClass(TEXT("/Script/PCG.PCGApplyHierarchySettings")));
    SettingsClassMap.Add(TEXT("ClusterElement"),
                         FindClass(TEXT("/Script/PCG.PCGClusterSettings")));
    SettingsClassMap.Add(
        TEXT("CollapsePoints"),
        FindClass(TEXT("/Script/PCG.PCGCollapsePointsSettings")));
    SettingsClassMap.Add(
        TEXT("CombinePoints"),
        FindClass(TEXT("/Script/PCG.PCGCombinePointsSettings")));
    SettingsClassMap.Add(
        TEXT("FindConvexHull2D"),
        FindClass(TEXT("/Script/PCG.PCGConvexHull2DSettings")));
    SettingsClassMap.Add(
        TEXT("CreatePointsSphere"),
        FindClass(TEXT("/Script/PCG.PCGCreatePointsSphereSettings")));
    SettingsClassMap.Add(
        TEXT("CreateSpline"),
        FindClass(TEXT("/Script/PCG.PCGCreateSplineSettings")));
    SettingsClassMap.Add(
        TEXT("DensityRemap"),
        FindClass(TEXT("/Script/PCG.PCGDensityRemapSettings")));
    SettingsClassMap.Add(
        TEXT("GenerateSeed"),
        FindClass(TEXT("/Script/PCG.PCGGenerateSeedSettings")));
    SettingsClassMap.Add(TEXT("GetBounds"),
                         FindClass(TEXT("/Script/PCG.PCGGetBoundsSettings")));
    SettingsClassMap.Add(TEXT("MutateSeed"),
                         FindClass(TEXT("/Script/PCG.PCGMutateSeedSettings")));
    SettingsClassMap.Add(
        TEXT("NormalToDensity"),
        FindClass(TEXT("/Script/PCG.PCGNormalToDensitySettings")));
    SettingsClassMap.Add(
        TEXT("PartitionByActorDataLayers"),
        FindClass(TEXT("/Script/PCG.PCGPartitionByActorDataLayersSettings")));
    SettingsClassMap.Add(
        TEXT("ResetPointCenter"),
        FindClass(TEXT("/Script/PCG.PCGResetPointCenterSettings")));
    SettingsClassMap.Add(
        TEXT("SampleTexture"),
        FindClass(TEXT("/Script/PCG.PCGSampleTextureSettings")));
    SettingsClassMap.Add(
        TEXT("SortAttributes"),
        FindClass(TEXT("/Script/PCG.PCGSortAttributesSettings")));
    SettingsClassMap.Add(TEXT("SplitPoints"),
                         FindClass(TEXT("/Script/PCG.PCGSplitPointsSettings")));
    SettingsClassMap.Add(
        TEXT("SubdivideSpline"),
        FindClass(TEXT("/Script/PCG.PCGSubdivideSplineSettings")));
    SettingsClassMap.Add(TEXT("ToPoint"),
                         FindClass(TEXT("/Script/PCG.PCGCollapseSettings")));
    SettingsClassMap.Add(
        TEXT("VisualizeAttribute"),
        FindClass(TEXT("/Script/PCG.PCGVisualizeAttributeSettings")));
  }

  UClass **FoundClass = SettingsClassMap.Find(NodeTypeName);
  if (FoundClass && *FoundClass) {
    return *FoundClass;
  }

  return nullptr;
}

UClass *FPCGGraphActions::FindClass(const FString &ClassPath) {
  UClass *FoundClass = FindFirstObject<UClass>(*ClassPath);
  if (!FoundClass) {
    FoundClass = LoadObject<UClass>(nullptr, *ClassPath);
  }
  return FoundClass;
}

UEdGraphPin *
FPCGGraphActions::FindCompatiblePin(UEdGraphNode *Node, UEdGraphPin *TargetPin,
                                    EEdGraphPinDirection Direction) {
  if (!Node || !TargetPin) {
    return nullptr;
  }

  for (UEdGraphPin *Pin : Node->Pins) {
    if (!Pin || Pin->Direction != Direction) {
      continue;
    }

    if (FPCGGraphActions::ArePinsCompatible(Pin, TargetPin)) {
      return Pin;
    }
  }

  return nullptr;
}

bool FPCGGraphActions::ArePinsCompatible(UEdGraphPin *PinA, UEdGraphPin *PinB) {
  if (!PinA || !PinB) {
    return false;
  }

  if (PinA->Direction == PinB->Direction) {
    return false;
  }

  if (PinA->PinType != PinB->PinType) {
    return false;
  }

  return true;
}

void FPCGGraphActions::CreatePinConnection(UEdGraphPin *OutputPin,
                                           UEdGraphPin *InputPin) {
  if (!OutputPin || !InputPin) {
    return;
  }

  const UEdGraphSchema *Schema = OutputPin->GetSchema();
  if (!Schema) {
    return;
  }

  Schema->TryCreateConnection(OutputPin, InputPin);
}

#undef LOCTEXT_NAMESPACE
