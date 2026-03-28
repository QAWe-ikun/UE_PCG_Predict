INTENT_MAPPING = {
    'Forest': {
        'preferred_sources': ['SurfaceSampler', 'GetLandscapeData'],
        'preferred_processors': ['DensityFilter', 'TransformPoints'],
    },
    'City': {
        'preferred_sources': ['GridGenerator', 'SplineSampler'],
        'preferred_processors': ['AttributeSet', 'RandomSelection'],
    }
}

def get_intent_bonus(node_type, intent_vector):
    return 0.15 if intent_vector is not None else 0.0
