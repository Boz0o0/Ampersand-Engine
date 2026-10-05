# ECS

!!! info "Work in progress"
    The ECS is not implemented yet. This page describes the planned design and will be updated as the code is written.

The engine uses a bitset-based ECS, based on the design described by [Austin Morlan](https://austinmorlan.com/posts/entity_component_system/).
For why we chose it, see [Design decisions](../decisions.md#ecs-bitset-based).

## Concepts

| Concept       | What it is                                                                  |
| ------------- | --------------------------------------------------------------------------- |
| **Entity**    | A plain ID. It has no data or behaviour of its own.                         |
| **Component** | A plain data struct (position, velocity, sprite…), attached to an entity.   |
| **Signature** | A fixed-size bitset per entity, with one bit per component type it has.     |
| **System**    | Logic that runs on every entity whose signature matches the components it needs. |

## How entities are matched to systems

Each component type gets an index in the bitset. Each system also has a signature: the set of components it needs.

An entity belongs to a system when all the bits of the system's signature are set in the entity's signature:

```cpp
(entitySignature & systemSignature) == systemSignature
```

This check is a single bitwise operation, whatever the number of components.
