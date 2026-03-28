from pcg_predict.intent.parser import IntentParser

def test_intent_parser():
    parser = IntentParser()
    result = parser.parse("茂密森林")
    assert result['vector'].shape == (64,)
    assert 'Forest' in result['scene']
    print("Intent parser test passed")

if __name__ == '__main__':
    test_intent_parser()
