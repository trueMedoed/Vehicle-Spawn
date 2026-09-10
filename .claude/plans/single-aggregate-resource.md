# Context

VBT уже является единственным владельцем per-prefab bounds. `ME_Vehicle_Spawn_Test` читает VBT Candidate и формирует aggregate schema v4, но сейчас сохраняет результат в отдельный staged resource, тогда как runtime preview читает canonical resource. Пользователь решил использовать Git для истории и diff, поэтому Test должен хранить один canonical aggregate config. Production addon не изменяется.

# Implementation

1. Переключить Test generator со staged path/GUID на canonical `{1C3AE4A8F2630BF7}Configs/Generated/ME_VehicleBoundsSnapshot.conf`, сохранив VBT validation, aggregate semantics, sorting и reload-validation.
2. Назначать serialized entries уникальные безопасные имена `<FactionKey>_<VehicleType>`.
3. Исправить canonical `.meta`, удалить staged `.conf/.meta`; обновлять `resourceDatabase.rdb` только через Workbench.
4. Обновить Test regression guide и root README под single-resource/Git workflow.
5. Выполнить Workbench rebuild, два deterministic generator run, Test/VBT validation, Git checks и cleanup; production не изменять.
