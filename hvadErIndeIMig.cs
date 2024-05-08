using System.Collections;
using System.Collections.Generic;
using UnityEngine;

public class hvadErIndeIMig : MonoBehaviour
{
    public string othertag = "";
    string currentTag = "";
    public GameObject SpilHaandtere;
    public spilHaandtere haandtere;
    public GameObject rightAwnser;
    public GameObject wrongAwnser;

    // Start is called before the first frame update
    void Start()
    {
        
    }

    // Update is called once per frame
    void Update()
    {
        Debug.Log(tag);
    }
    
    void OnTriggerEnter(Collider other) 
    {
        currentTag = other.tag;
        if (other.gameObject.CompareTag(othertag))
        {
            Debug.Log(currentTag+" ER INDE I MIG!!!");
            haandtere.antalBrikker = haandtere.antalBrikker + 1;
            rightAwnser.SetActive(true);
        }
        else
        {
            wrongAwnser.SetActive(true);
        }

    }
    void OnTriggerExit(Collider other)
    {
        if (other.gameObject.CompareTag(othertag))
        {
            Debug.Log(currentTag+" ER UDE AF MIG!!!");
            haandtere.antalBrikker = haandtere.antalBrikker - 1;
            rightAwnser.SetActive(false);
        }
        else
        {
            wrongAwnser.SetActive(false);
        }
    }
}
