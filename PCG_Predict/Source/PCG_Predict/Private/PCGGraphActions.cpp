#include "PCGGraphActions.h"
#include "EdGraph/EdGraph.h"
#include "EdGraph/EdGraphNode.h"
#include "EdGraph/EdGraphPin.h"
#include "EdGraph/EdGraphSchema.h"
#include "GraphEditor.h"
#include "SGraphPanel.h"
#include "UObject/UObjectIterator.h"
#include "Misc/FileHelper.h"
#include "Serialization/JsonSerializer.h"
#include "Dom/JsonObject.h"
#include "GraphEditorActions.h"

// PCG 核心头文件
#include "PCGGraph.h"
#include "PCGNode.h"
#include "PCGSettings.h"

// PCG 编辑器节点类（需要私有路径访问）
#include "PCGEditorGraph.h"
#include "Nodes/PCGEditorGraphNode.h"

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

  // 1. 查找 Settings 类
  UClass *SettingsClass = FindPCGSettingsClass(NodeTypeName);
  if (!SettingsClass) {
    UE_LOG(LogTemp, Error,
           TEXT("[PCGGraphActions] Settings class not found for: %s"),
           *NodeTypeName);
    return false;
  }

  UE_LOG(LogTemp, Log, TEXT("[PCGGraphActions] Found settings class: %s"),
         *SettingsClass->GetName());

  // 2. 转换为 PCGEditorGraph
  UPCGEditorGraph* PCGEditorGraph = Cast<UPCGEditorGraph>(EdGraph);
  if (!PCGEditorGraph) {
    UE_LOG(LogTemp, Error, TEXT("[PCGGraphActions] EdGraph is not a UPCGEditorGraph"));
    return false;
  }

  // 3. 使用事务创建节点（支持 Undo/Redo）
  const FScopedTransaction Transaction(
      LOCTEXT("PCGGraphActions_CreateNode", "Create PCG Node"));

  PCGEditorGraph->Modify();

  // 4. 创建数据层节点
  UPCGSettings* DefaultSettings = nullptr;
  UPCGNode *NewPCGNode = PCGGraph->AddNodeOfType(SettingsClass, DefaultSettings);
  if (!NewPCGNode) {
    UE_LOG(LogTemp, Error,
           TEXT("[PCGGraphActions] Failed to add node to graph"));
    return false;
  }

  UE_LOG(LogTemp, Log, TEXT("[PCGGraphActions] PCG Node created"));

  // 5. 创建可视化层节点
  FGraphNodeCreator<UPCGEditorGraphNode> NodeCreator(*PCGEditorGraph);
  UPCGEditorGraphNode* NewEdGraphNode = NodeCreator.CreateUserInvokedNode(true);

  NewEdGraphNode->Construct(NewPCGNode);
  NewEdGraphNode->NodePosX = SpawnLocation.X;
  NewEdGraphNode->NodePosY = SpawnLocation.Y;
  NodeCreator.Finalize();

  // 6. 同步位置到数据层
  NewPCGNode->PositionX = SpawnLocation.X;
  NewPCGNode->PositionY = SpawnLocation.Y;

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

  UPCGSettings* DefaultSettings = nullptr;
  UPCGNode *NewNode = Graph->AddNodeOfType(SettingsClass, DefaultSettings);
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

// 辅助函数：安全添加到映射表，过滤掉 nullptr
void SafeAddToMap(TMap<FString, UClass*>& Map, const FString& Key, UClass* Class) {
  if (Class) {
    Map.Add(Key, Class);
  }
}

UClass *FPCGGraphActions::FindPCGSettingsClass(const FString &NodeTypeName) {
  static TMap<FString, UClass *> SettingsClassMap;
  static bool bHasUpdatedJson = false;

  if (SettingsClassMap.Num() == 0) {
    UE_LOG(LogTemp, Log, TEXT("[PCGGraphActions] Building class map from actual UE classes..."));

    // 第一步：枚举所有实际存在的 PCG Settings 类
    TMap<FString, FString> ClassNameToPath; // ClassName -> FullPath
    for (TObjectIterator<UClass> It; It; ++It) {
      UClass* Class = *It;
      if (Class && Class->IsChildOf(UPCGSettings::StaticClass()) && !Class->HasAnyClassFlags(CLASS_Abstract)) {
        FString FullPath = Class->GetPathName();
        FString ClassName = Class->GetName();
        ClassNameToPath.Add(ClassName, FullPath);
      }
    }

    UE_LOG(LogTemp, Log, TEXT("[PCGGraphActions] Found %d PCG Settings classes"), ClassNameToPath.Num());

    // 第二步：从 JSON 读取节点名称和类名的映射
    FString ConfigPath = TEXT("D:/experiment/UE_PCG_Predict/python/config/node_registry.json");
    FString JsonString;

    if (FFileHelper::LoadFileToString(JsonString, *ConfigPath)) {
      TSharedPtr<FJsonObject> JsonObject;
      TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(JsonString);

      if (FJsonSerializer::Deserialize(Reader, JsonObject) && JsonObject.IsValid()) {
        const TArray<TSharedPtr<FJsonValue>>* NodesArray;
        if (JsonObject->TryGetArrayField(TEXT("nodes"), NodesArray)) {

          // 创建可修改的副本用于更新
          TArray<TSharedPtr<FJsonValue>> ModifiedNodesArray;

          // 第三步：更新 JSON 中的 class_path 字段
          for (const TSharedPtr<FJsonValue>& NodeValue : *NodesArray) {
            const TSharedPtr<FJsonObject>* NodeObj;
            if (NodeValue->TryGetObject(NodeObj)) {
              FString Name, ClassName;
              if ((*NodeObj)->TryGetStringField(TEXT("name"), Name) &&
                  (*NodeObj)->TryGetStringField(TEXT("class"), ClassName)) {

                // 创建可修改的副本
                TSharedPtr<FJsonObject> ModifiedNodeObj = MakeShareable(new FJsonObject(**NodeObj));

                // 查找实际的类路径
                FString* FoundPath = ClassNameToPath.Find(ClassName);
                if (FoundPath) {
                  // 更新 JSON 对象
                  ModifiedNodeObj->SetStringField(TEXT("class_path"), *FoundPath);

                  // 加载类并添加到映射表
                  UClass* FoundClass = FindClass(**FoundPath);
                  if (FoundClass) {
                    SettingsClassMap.Add(Name, FoundClass);
                  }
                } else {
                  // 类不存在，标记为 null
                  ModifiedNodeObj->SetStringField(TEXT("class_path"), TEXT(""));
                  UE_LOG(LogTemp, Verbose, TEXT("[PCGGraphActions] Class not found: %s"), *ClassName);
                }

                ModifiedNodesArray.Add(MakeShareable(new FJsonValueObject(ModifiedNodeObj)));
              }
            }
          }

          // 更新 JSON 对象中的 nodes 数组
          JsonObject->SetArrayField(TEXT("nodes"), ModifiedNodesArray);

          // 第四步：保存更新后的 JSON
          if (!bHasUpdatedJson) {
            FString OutputString;
            TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&OutputString);
            if (FJsonSerializer::Serialize(JsonObject.ToSharedRef(), Writer)) {
              if (FFileHelper::SaveStringToFile(OutputString, *ConfigPath, FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM)) {
                UE_LOG(LogTemp, Log, TEXT("[PCGGraphActions] Updated node_registry.json with class paths"));
                bHasUpdatedJson = true;
              }
            }
          }
        }
      }
    }

    UE_LOG(LogTemp, Log, TEXT("[PCGGraphActions] Loaded %d valid node classes"), SettingsClassMap.Num());
  }

  // 旧的硬编码映射作为后备（如果 JSON 加载失败）
  if (SettingsClassMap.Num() == 0) {
    UE_LOG(LogTemp, Warning, TEXT("[PCGGraphActions] JSON loading failed, using fallback mappings"));
    SafeAddToMap(SettingsClassMap, TEXT("Difference"),
        FindClass(TEXT("/Script/PCG.PCGDifferenceSettings")));
    SafeAddToMap(SettingsClassMap, TEXT("Union"),
        FindClass(TEXT("/Script/PCG.PCGUnionSettings")));
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

  // 不输出警告，静默失败
  if (!FoundClass) {
    UE_LOG(LogTemp, Verbose, TEXT("[PCGGraphActions] Class not found: %s"), *ClassPath);
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