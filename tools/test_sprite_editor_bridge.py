import json
import queue
import time
import unittest
from types import SimpleNamespace
from sprite_editor_bridge import EditorBridge
from sprite_material import Project


class Root:
    def after(self,*args): return 1


class BridgeTests(unittest.TestCase):
    def setUp(self):
        self.opened=[]
        self.bridge=EditorBridge.__new__(EditorBridge)
        self.bridge.requests=queue.Queue()
        self.bridge.editor=SimpleNamespace(root=Root(),project=Project(),project_session='current',index=0,dirty=False,busy=False,collection=self.opened.append)
        self.bridge.collections=lambda:[dict(id='bowman',name='Bowman',path='bowman.collection')]

    def call(self,path,body=None,expired=False):
        reply=queue.Queue()
        self.bridge.requests.put((path,body,time.monotonic()+(-1 if expired else 5),reply))
        self.bridge.poll();return reply.get()

    def test_pbr_state_and_command(self):
        from PIL import Image
        from sprite_material import Frame
        e=self.bridge.editor;e.project=Project([Frame('a',Image.new('RGBA',(2,2)))])
        e.refresh=lambda:None
        state=json.loads(self.call('/state')[2]);self.assertFalse(state['frames'][0]['pbr_enabled'])
        body=dict(project_session=state['project_session'],revision=state['revision'],indices=[0],enabled=True)
        self.assertEqual(self.call('/pbr-set',body)[0],200)
        self.assertTrue(json.loads(self.call('/state')[2])['frames'][0]['pbr_enabled'])
        self.assertTrue(e.dirty)
        self.assertEqual(self.call('/pbr-set',body)[0],409)

    def test_history_session(self):
        self.bridge.editor.project.history=[dict(operation='brush',channel='thickness')]
        self.assertEqual(json.loads(self.call('/history/current')[2])['history'][0]['operation'],'brush')
        self.assertEqual(self.call('/history/old')[0],404)
        self.assertEqual(json.loads(self.call('/state')[2])['history_count'],1)

    def test_open_and_guards(self):
        body=dict(collection_id='bowman',project_session='current')
        self.assertEqual(self.call('/open-collection',body)[0],202)
        self.assertEqual(self.opened,['bowman.collection']);self.opened.clear()
        self.bridge.editor.dirty=True
        self.assertEqual(self.call('/open-collection',body)[0],409)
        self.bridge.editor.dirty=False
        self.assertEqual(self.call('/open-collection',dict(body,project_session='old'))[0],409)
        self.assertEqual(self.call('/open-collection',body,True)[0],408)
        self.assertEqual(self.opened,[])

    def test_state_and_stale_image(self):
        status,_,data=self.call('/state');self.assertEqual(status,200)
        from sprite_material_hints import HINT_COUNT
        self.assertEqual(len(json.loads(data)['material_hints']),HINT_COUNT)
        self.assertEqual(self.call('/source/old/0/image.png')[0],404)
        self.assertEqual(self.call('/state',{})[0],404)


if __name__=='__main__':unittest.main()
